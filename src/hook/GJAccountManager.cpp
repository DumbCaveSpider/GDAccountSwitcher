#include <Geode/Geode.hpp>
#include <Geode/modify/GJAccountManager.hpp>
#include <matjson.hpp>
#include "../PendingSwitch.hpp"

using namespace geode::prelude;
using namespace geode::utils;

class $modify(AWGJAccountManager, GJAccountManager) {
     public:
      void onLoginAccountCompleted(gd::string response, gd::string tag) {

            std::string originalUsername = account::originalUsername;
            std::string originalGJP2 = account::originalGJP2;
            int originalAccountID = account::originalAccountID;
            int originalUserID = account::originalUserID;
            bool isSwitchingAccount = account::isSwitchingAccount;

            log::debug("Original account before switch: username={} gjp2={} accountID={} userID={}",
                  originalUsername,
                  originalGJP2,
                  originalAccountID,
                  originalUserID);
            GJAccountManager::onLoginAccountCompleted(response, tag);

            if (!isSwitchingAccount) return; // treat like a normal login if not switching
            
            // for debugging
            log::debug("login complete response: {}", response);

            bool linked = false;
            std::string s = response;

            geode::utils::string::trim(s);

            std::string chosenUsername = "";
            std::string chosenGJP2 = "";

            auto pos = s.find(',');
            if (pos != std::string::npos) {
                  std::string strAccountID = s.substr(0, pos);
                  std::string strUserID = s.substr(pos + 1);
                  geode::utils::string::trim(strAccountID);
                  geode::utils::string::trim(strUserID);
                  int accountID = geode::utils::numFromString<int>(strAccountID).unwrapOr(0);
                  int userID = geode::utils::numFromString<int>(strUserID).unwrapOr(0);

                  auto path = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID() / "accounts.json";

                  matjson::Value doc = matjson::Value::object();
                  if (auto res = geode::utils::file::readJson(path); res) {
                        doc = res.unwrap();
                  }

                  matjson::Value accountsVal;
                  if (doc.contains("accounts")) {
                        accountsVal = doc["accounts"];
                  } else {
                        accountsVal = doc;
                  }

                  std::vector<matjson::Value> accounts;
                  if (accountsVal.isArray()) {
                        if (auto r = accountsVal.as<std::vector<matjson::Value>>(); r) {
                              accounts = r.unwrap();
                        }
                  } else if (accountsVal.isObject()) {
                        if (accountsVal.contains("username")) {
                              accounts.push_back(accountsVal);
                        }
                  }

                  chosenUsername = "";
                  chosenGJP2 = "";

                  if (!account::pendingUsername.empty()) {
                        chosenUsername = account::pendingUsername;
                        if (!account::pendingGJP2.empty()) {
                              chosenGJP2 = account::pendingGJP2;
                        } else {
                              // try to find gjp2 from file by matching username
                              for (auto const& a : accounts) {
                                    if (auto s = a["username"].asString(); s && s.unwrap() == chosenUsername) {
                                          if (auto g = a["gjp2"].asString(); g) chosenGJP2 = g.unwrap();
                                          break;
                                    }
                              }
                        }
                        // clear pending values after consuming
                        account::pendingUsername.clear();
                        account::pendingGJP2.clear();
                  } else if (!account::originalUsername.empty()) {
                        // revert attempt
                        chosenUsername = account::originalUsername;
                        chosenGJP2 = account::originalGJP2;
                        // if no GJP2 stored, try to find it in file
                        if (chosenGJP2.empty()) {
                              for (auto const& a : accounts) {
                                    if (auto s = a["username"].asString(); s && s.unwrap() == chosenUsername) {
                                          if (auto g = a["gjp2"].asString(); g) chosenGJP2 = g.unwrap();
                                          break;
                                    }
                              }
                        }
                  } else if (!accounts.empty()) {
                        // if only one account, use that instead yea
                        if (auto u = accounts[0]["username"].asString(); u) chosenUsername = u.unwrap();
                        if (auto g = accounts[0]["gjp2"].asString(); g) chosenGJP2 = g.unwrap();
                  } else {
                        // last resort
                        chosenUsername = this->m_username;
                        chosenGJP2 = this->m_GJP2;
                  }

                  if (chosenUsername.empty()) {
                        log::warn("Account invalid for switch, no credentials available to link");
                        Notification::create("Account Switch failed: no credentials available.", NotificationIcon::Error)->show();
                        linked = false;
                  } else {
                        this->linkToAccount(chosenUsername, chosenGJP2, accountID, userID);
                        log::debug("linkToAccount called with username={} accountID={} userID={}", chosenUsername, accountID, userID);
                        linked = true;

                        // clear pending/original saved credentials
                        account::pendingUsername.clear();
                        account::pendingGJP2.clear();
                        account::originalUsername.clear();
                        account::originalGJP2.clear();
                        account::originalAccountID = 0;
                        account::originalUserID = 0;
                  }

                  // clear pending/original saved credentials
                  account::pendingUsername.clear();
                  account::pendingGJP2.clear();
                  account::originalUsername.clear();
                  account::originalGJP2.clear();
                  account::originalAccountID = 0;
                  account::originalUserID = 0;
            } else {
                  log::warn("Failed to parse login response '{}'", response);
                  linked = false;
                  account::isSwitchingAccount = false;
            }

            if (linked) {
                  log::debug("account switch to {} complete", chosenUsername);
                  Notification::create(std::string("Switched to ") + chosenUsername, NotificationIcon::Success)->show();
                  account::isSwitchingAccount = false;
            }

            if (!linked) {
                  log::debug("no account/user IDs found to link");

                  // clear any pending requested switch
                  account::pendingUsername.clear();
                  account::pendingGJP2.clear();

                  // attempt to revert to the original account
                  if (!account::originalUsername.empty() && !account::originalGJP2.empty()) {
                        log::info("Reverting to original account {}", account::originalUsername);
                        this->unlinkFromAccount();
                        this->loginAccount(account::originalUsername, account::originalGJP2);
                        Notification::create("Account Switch failed, reverting to " + account::originalUsername + ".", NotificationIcon::Error)->show();
                        account::isSwitchingAccount = false;
                        return;
                  } else {
                        log::warn("No original credentials available to revert to");
                        Notification::create("Account Switch failed and no original account to revert to.", NotificationIcon::Error)->show();
                        account::isSwitchingAccount = false;
                        return;
                  }
            }
      }
};
