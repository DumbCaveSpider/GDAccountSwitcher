#include "AccountSwitch.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <optional>

using namespace geode::prelude;

namespace account {
    namespace {
        enum class Stage {
            Idle,
            BackingUp,
            LoggingIn,
            LoadingCloud
        };

        struct SwitchState {
            bool active = false;
            Stage stage = Stage::Idle;

            std::string targetUsername;
            std::string targetGJP2;

            std::string originalUsername;
            std::string originalGJP2;
            int originalAccountID = 0;
            int originalUserID = 0;

            SwitchOptions options;
            StatusCallback onStatus;
            FinishCallback onFinished;
        };

        SwitchState g_state;

        constexpr int kBackupServer = 1;
        constexpr int kSyncServer = 2;

        bool inStage(Stage stage) {
            return g_state.active && g_state.stage == stage;
        }

        void setStatus(std::string const& status) {
            log::debug("switch: {}", status);
            if (g_state.onStatus) g_state.onStatus(status);
        }

        void finish(bool success) {
            auto onFinished = std::move(g_state.onFinished);
            g_state = SwitchState{};
            if (onFinished) onFinished(success);
        }

        bool hasOriginalAccount() {
            return !g_state.originalUsername.empty() && !g_state.originalGJP2.empty() && g_state.originalAccountID > 0;
        }

        web::WebFuture postForm(std::string const& url, std::string const& body) {
            return web::WebRequest()
                .userAgent("")
                .header("Content-Type", "application/x-www-form-urlencoded")
                .bodyString(body)
                .post(url);
        }

        std::optional<std::string> responseBody(web::WebResponse const& res, std::string& reason) {
            if (!res.ok()) {
                reason = res.code() > 0 ? fmt::format("couldn't reach the server (HTTP {})", res.code())
                                        : "couldn't reach the server";
                return std::nullopt;
            }
            auto body = res.string().unwrapOr("");
            utils::string::trimIP(body);
            return body;
        }

        void fetchAccountServer(int type, Stage stage, std::function<void(std::optional<std::string>, std::string const&)> then) {
            auto body = fmt::format("accountID={}&type={}&secret=Wmfd2893gb7", GJAccountManager::get()->m_accountID, type);
            async::spawn(
                postForm("https://www.boomlings.com/database/getAccountURL.php", body),
                [stage, then = std::move(then)](web::WebResponse res) {
                    if (!inStage(stage)) return;
                    std::string reason;
                    auto serverURL = responseBody(res, reason);
                    if (!serverURL) return then(std::nullopt, reason);
                    if (serverURL->empty() || serverURL->starts_with("-")) {
                        return then(std::nullopt, "couldn't find the account's save server");
                    }
                    while (serverURL->ends_with('/')) serverURL->pop_back();
                    then(std::move(serverURL), "");
                });
        }

        std::string describeAccountCode(int code) {
            switch (code) {
                case -1: return "the server rejected the request";
                case -2: return "the login is no longer valid";
                case -4: return "the save is too large";
                case -5:
                case -11: return "invalid login info";
                case -6: return "something went wrong on the server";
                default: return fmt::format("the server returned {}", code);
            }
        }

        std::optional<std::pair<int, int>> parseLoginIDs(std::string response) {
            utils::string::trimIP(response);
            auto pos = response.find(',');
            if (pos == std::string::npos) return std::nullopt;

            auto accountStr = response.substr(0, pos);
            auto userStr = response.substr(pos + 1);
            utils::string::trimIP(accountStr);
            utils::string::trimIP(userStr);

            int accountID = utils::numFromString<int>(accountStr).unwrapOr(0);
            int userID = utils::numFromString<int>(userStr).unwrapOr(0);
            if (accountID <= 0) return std::nullopt;
            return std::make_pair(accountID, userID);
        }

        std::string describeLoginError(std::string response) {
            utils::string::trimIP(response);
            if (response.empty()) return "no response from the server";
            if (response == "-1") return "login failed (wrong password or no connection)";
            if (response == "-11") return "invalid username or password";
            if (response == "-12") return "the account is disabled";
            if (response == "-13") return "the account is linked to a different Steam account";
            return fmt::format("the server returned {}", response);
        }

        void restoreOriginal(std::string const& reason) {
            log::warn("account switch to {} failed: {}", g_state.targetUsername, reason);

            if (hasOriginalAccount()) {
                setStatus(fmt::format("Switch failed, restoring {}...", g_state.originalUsername));
                GJAccountManager::get()->linkToAccount(
                    g_state.originalUsername,
                    g_state.originalGJP2,
                    g_state.originalAccountID,
                    g_state.originalUserID);
                Notification::create(
                    fmt::format("Switch failed: {}. Restored {}.", reason, g_state.originalUsername),
                    NotificationIcon::Error)
                    ->show();
            } else {
                Notification::create(fmt::format("Switch failed: {}.", reason), NotificationIcon::Error)->show();
            }

            finish(false);
        }

        void beginLogin() {
            g_state.stage = Stage::LoggingIn;
            auto gjam = GJAccountManager::get();

            if (!g_state.originalUsername.empty()) {
                setStatus(fmt::format("Logging out of {}...", g_state.originalUsername));
            }
            gjam->unlinkFromAccount();

            setStatus(fmt::format("Logging in to {}...", g_state.targetUsername));
            gjam->loginAccount(g_state.targetUsername, g_state.targetGJP2);
        }
        std::string buildSaveData() {
            auto gjam = GJAccountManager::get();
            auto gm = GameManager::get();

            std::string gjp2 = gjam->m_GJP2;
            gjam->m_GJP2 = "0";
            gm->m_quickSave = true;
            gm->m_isBackingUp = true;
            std::string gameData = gm->getCompressedSaveString();
            gm->m_quickSave = false;
#if !defined(GEODE_IS_IOS)
            // only fixes up the saved level order; not bound on iOS
            LocalLevelManager::get()->updateLevelOrder();
#endif
            std::string levelData = LocalLevelManager::get()->getCompressedSaveString();
            gm->m_isBackingUp = false;
            gjam->m_GJP2 = gjp2;

            return gameData + ";" + levelData;
        }

        void finishBackup(bool success, std::string const& reason) {
            if (!inStage(Stage::BackingUp)) return;
            GJAccountManager::get()->m_activeDownloads->removeObjectForKey("bak_account");

            if (!success) {
                log::warn("backup of {} failed, switching anyway: {}", g_state.originalUsername, reason);
                Notification::create(
                    fmt::format("Couldn't back up {}: {}. Switching anyway.", g_state.originalUsername, reason),
                    NotificationIcon::Warning)
                    ->show();
                beginLogin();
                return;
            }

            log::info("backed up {}", g_state.originalUsername);
#if !defined(GEODE_IS_IOS)
            GameLevelManager::get()->makeTimeStamp("bak_account");
#endif
            if (auto levels = LocalLevelManager::get()->m_localLevels) {
                for (auto level : CCArrayExt<GJGameLevel*>(levels)) {
                    level->m_hasBeenModified = false;
                }
            }
            beginLogin();
        }

        void uploadBackup(std::string serverURL) {
            setStatus(fmt::format("Backing up {}'s data...", g_state.originalUsername));

            auto body = std::string(GameLevelManager::get()->getBasePostString()) +
                        "&saveData=" + buildSaveData() + "&secret=Wmfv3899gc9";
            async::spawn(
                postForm(serverURL + "/database/accounts/backupGJAccountNew.php", body),
                [](web::WebResponse res) {
                    if (!inStage(Stage::BackingUp)) return;
                    std::string reason;
                    auto response = responseBody(res, reason);
                    if (!response) return finishBackup(false, reason);

                    int code = utils::numFromString<int>(*response).unwrapOr(0);
                    if (*response != "1" && code < 0) return finishBackup(false, describeAccountCode(code));
                    finishBackup(true, "");
                });
        }

        void startBackup() {
            g_state.stage = Stage::BackingUp;
            auto gjam = GJAccountManager::get();

            if (gjam->m_activeDownloads->objectForKey("burl_account") ||
                gjam->m_activeDownloads->objectForKey("bak_account")) {
                // someone else's backup owns that flag, so skip ours without touching it
                log::warn("backup of {} skipped: another backup is in progress", g_state.originalUsername);
                Notification::create(
                    fmt::format("Couldn't back up {}: another backup is in progress. Switching anyway.",
                        g_state.originalUsername),
                    NotificationIcon::Warning)
                    ->show();
                beginLogin();
                return;
            }
            gjam->m_activeDownloads->setObject(CCNode::create(), "bak_account");

            setStatus("Fetching Account Backup Server URL...");
            fetchAccountServer(kBackupServer, Stage::BackingUp, [](std::optional<std::string> url, std::string const& reason) {
                if (!url) return finishBackup(false, reason);
                uploadBackup(std::move(*url));
            });
        }

        std::string describeSyncError(BackupAccountError error, int response) {
            switch (static_cast<int>(error)) {
                case -2: return describeAccountCode(-2);
                case -3: return describeAccountCode(response);
                default: return "sync failed";
            }
        }

        void finishCloudLoad(bool loaded, std::string const& reason);

        struct CloudLoadListener : GJAccountSyncDelegate {
            GJAccountSyncDelegate* prevSync = nullptr;

            void install() {
                auto gjam = GJAccountManager::get();
                prevSync = gjam->m_syncDelegate;
                gjam->m_syncDelegate = this;
            }

            void uninstall() {
                auto gjam = GJAccountManager::get();
                if (gjam->m_syncDelegate == this) gjam->m_syncDelegate = prevSync;
                prevSync = nullptr;
            }

            void syncAccountFinished() override {
                finishCloudLoad(true, "");
            }
            void syncAccountFailed(BackupAccountError error, int response) override {
                finishCloudLoad(false, describeSyncError(error, response));
            }
        };

        CloudLoadListener g_cloudListener;

        void finishCloudLoad(bool loaded, std::string const& reason) {
            if (!inStage(Stage::LoadingCloud)) return;
            g_cloudListener.uninstall();
            GJAccountManager::get()->m_activeDownloads->removeObjectForKey("sync_account");

            if (loaded) {
                log::info("loaded cloud data for {}", g_state.targetUsername);
                Notification::create(
                    fmt::format("Switched to {} and loaded cloud data", g_state.targetUsername),
                    NotificationIcon::Success)
                    ->show();
            } else {
                log::warn("couldn't load cloud data for {}: {}", g_state.targetUsername, reason);
                Notification::create(
                    fmt::format("Switched to {}, but loading cloud data failed: {}. Use Load in the account menu.",
                        g_state.targetUsername,
                        reason),
                    NotificationIcon::Warning)
                    ->show();
            }
            finish(true);
        }

        void applySyncResponse(std::string response) {
            setStatus(fmt::format("Loading {}'s save data...", g_state.targetUsername));
            GJAccountManager::get()->onSyncAccountCompleted(std::move(response), "SyncAccount");
        }

        void downloadSave(std::string serverURL) {
            setStatus(fmt::format("Downloading {}'s save data...", g_state.targetUsername));

            auto body = std::string(GameLevelManager::get()->getBasePostString()) + "&secret=Wmfv3899gc9";
            async::spawn(
                postForm(serverURL + "/database/accounts/syncGJAccountNew.php", body),
                [](web::WebResponse res) {
                    if (!inStage(Stage::LoadingCloud)) return;
                    std::string reason;
                    auto response = responseBody(res, reason);
                    if (!response) return finishCloudLoad(false, reason);
                    applySyncResponse(std::move(*response));
                });
        }

        void startCloudLoad() {
            g_state.stage = Stage::LoadingCloud;
            auto gjam = GJAccountManager::get();

            if (gjam->m_activeDownloads->objectForKey("surl_account") ||
                gjam->m_activeDownloads->objectForKey("sync_account")) {
                finishCloudLoad(false, "a cloud load is already in progress");
                return;
            }
            gjam->m_activeDownloads->setObject(CCNode::create(), "sync_account");
            g_cloudListener.install();

            setStatus("Fetching Account Backup Server URL...");
            fetchAccountServer(kSyncServer, Stage::LoadingCloud, [](std::optional<std::string> url, std::string const& reason) {
                if (!url) return finishCloudLoad(false, reason);
                downloadSave(std::move(*url));
            });
        }
    }

    bool isSwitching() {
        return g_state.active;
    }

    bool startSwitch(std::string username, std::string gjp2, SwitchOptions options, StatusCallback onStatus, FinishCallback onFinished) {
        if (g_state.active) return false;

        auto gjam = GJAccountManager::get();

        g_state.active = true;
        g_state.targetUsername = std::move(username);
        g_state.targetGJP2 = std::move(gjp2);
        g_state.originalUsername = gjam->m_username;
        g_state.originalGJP2 = gjam->m_GJP2;
        g_state.originalAccountID = gjam->m_accountID;
        g_state.originalUserID = GameManager::get()->m_playerUserID.value();
        g_state.options = options;
        g_state.onStatus = std::move(onStatus);
        g_state.onFinished = std::move(onFinished);

        if (gjam->m_activeDownloads->objectForKey("login_account")) {
            log::warn("account switch to {} aborted: another login is in progress", g_state.targetUsername);
            Notification::create("Another login is already in progress, try again.", NotificationIcon::Error)->show();
            finish(false);
            return true;
        }

        if (hasOriginalAccount() && !g_state.options.skipBackup) {
            startBackup();
        } else {
            beginLogin();
        }
        return true;
    }

    void handleLoginResponse(std::string response) {
        if (!inStage(Stage::LoggingIn)) return;

        auto ids = parseLoginIDs(response);
        if (!ids) {
            restoreOriginal(describeLoginError(response));
            return;
        }

        auto [accountID, userID] = *ids;
        setStatus(fmt::format("Linking {}...", g_state.targetUsername));
        GJAccountManager::get()->linkToAccount(g_state.targetUsername, g_state.targetGJP2, accountID, userID);

        log::info("switched to account {} (accountID={}, userID={})", g_state.targetUsername, accountID, userID);
        if (g_state.options.skipCloudLoad) {
            Notification::create(fmt::format("Switched to {}", g_state.targetUsername), NotificationIcon::Success)->show();
            finish(true);
            return;
        }
        startCloudLoad();
    }
}
