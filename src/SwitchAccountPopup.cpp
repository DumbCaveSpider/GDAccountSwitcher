#include "SwitchAccountPopup.hpp"

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include <matjson.hpp>

#include "include/PendingSwitch.hpp"

using namespace geode::prelude;

SwitchAccountPopup* SwitchAccountPopup::create() {
    auto ret = new SwitchAccountPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool SwitchAccountPopup::init() {
    if (!Popup::init(400.f, 300.f))
        return false;

    setTitle("Switch Online Account");
    m_noElasticity = true;
    m_closeBtn->removeFromParent();

    m_listLayer = cue::ListNode::createComments({350.f, 200.f});
    m_listLayer->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height / 2 + 10});
    m_listLayer->setCellHeight(50.f);
    m_listLayer->getScrollLayer()->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoGrowAxis(0.f));
    m_mainLayer->addChild(m_listLayer);

    this->reloadAccountRows();

    // add button
    auto addBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Add", "goldFont.fnt", "GJ_button_01.png"), this, menu_selector(SwitchAccountPopup::onAdd));
    m_buttonMenu->addChild(addBtn);
    addBtn->setPosition({m_mainLayer->getContentSize().width / 2 + 70, 25.f});

    // cancel button
    auto cancelBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Cancel", "goldFont.fnt", "GJ_button_06.png"), this, menu_selector(SwitchAccountPopup::onClose));
    m_buttonMenu->addChild(cancelBtn);
    cancelBtn->setPosition({m_mainLayer->getContentSize().width / 2 - 50, 25.f});

    // info button
    auto infoBtn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this, menu_selector(SwitchAccountPopup::onInfo));
    m_buttonMenu->addChild(infoBtn);
    infoBtn->setPosition({m_mainLayer->getContentSize().width - 20.f,
        m_mainLayer->getContentSize().height - 20.f});

    // show saved files
    auto saveBtnSpr = CircleButtonSprite::
        createWithSpriteFrameName(  // @geode-ignore(unknown-resource)
            "geode.loader/save.png",
            1.f,
            CircleBaseColor::Green,
            CircleBaseSize::Medium);
    saveBtnSpr->setScale(0.55f);
    auto saveBtn = CCMenuItemSpriteExtra::create(
        saveBtnSpr, this, menu_selector(SwitchAccountPopup::onShowSavedFolder));
    saveBtn->setPosition({20.f, m_mainLayer->getContentSize().height - 20.f});
    m_buttonMenu->addChild(saveBtn);


    return true;
}

void SwitchAccountPopup::clearAccountRows() {
    if (!m_listLayer)
        return;

    m_listLayer->clear();
    m_accountNodes.clear();
    m_currentAccountNode = nullptr;
}

void SwitchAccountPopup::reloadAccountRows() {
    clearAccountRows();

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
        if (auto result = accountsVal.as<std::vector<matjson::Value>>(); result) {
            accounts = result.unwrap();
        }
    } else if (accountsVal.isObject()) {
        if (accountsVal.contains("username")) {
            accounts.push_back(accountsVal);
        }
    }

    std::string currentUsername = GJAccountManager::sharedState()->m_username;

    for (auto const& acc : accounts) {
        std::string uname = "";
        if (auto result = acc["username"].asString(); result) {
            uname = result.unwrap();
        }
        std::string gjp2 = "";
        if (auto result = acc["gjp2"].asString(); result) {
            gjp2 = result.unwrap();
        }

        if (uname.empty() || gjp2.empty())
            continue;

        this->addAccountRow(uname, gjp2, uname == currentUsername);
    }
    
    m_listLayer->getScrollLayer()->scrollToTop();
}

void SwitchAccountPopup::onShowSavedFolder(CCObject* sender) {
    auto path = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
    geode::utils::file::openFolder(path);
}

void SwitchAccountPopup::onInfo(CCObject* sender) {
    FLAlertLayer::create(
        "How to use",
        "Log in to your <cy>current account</c>, then click <cg>Add</c> to store "
        "it locally.\nTo switch accounts, simply click the <cg>Use</c> button "
        "next to the desired account.\nThe game will <cr>log out of your current "
        "account</c> and <cg>log back into the selected account</c> "
        "automatically.",
        "OK")
        ->show();
}

void SwitchAccountPopup::addAccountRow(const std::string& username,
    const std::string& gjp2,
    bool toggleDefault) {
    if (!m_listLayer)
        return;

    auto node = AccountNode::create(
        username,
        gjp2,
        toggleDefault,
        [this](AccountNode* node) { this->onAccountNodeDelete(node); },
        [this](AccountNode* node) { this->onAccountNodeSelect(node); });

    if (!node)
        return;

    auto cell = cue::ListCell::create(node, m_listLayer);
    m_listLayer->addListCell(cell);
    m_accountNodes.push_back(node);
}

void SwitchAccountPopup::onAdd(CCObject* sender) {
    // ask for confirmation and append the new account on yes
    auto path = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID() /
                "accounts.json";

    createQuickPopup(
        "Add Account",
        "Are you sure you want to add your <cg>current logged in</c> "
        "account?\n<cy>This will store your currently login Username and GJP2 "
        "(Password) locally on your device.</c>",
        "No",
        "Yes",
        [this, path](auto, bool yes) {
            if (!yes)
                return;

            // fetch current credentials at confirmation time
            std::string username = GJAccountManager::sharedState()->m_username;
            std::string gjp2 = GJAccountManager::sharedState()->m_GJP2;

            if (username.empty() || gjp2.empty()) {
                Notification::create("Error when getting current account.",
                    NotificationIcon::Error)
                    ->show();
                return;
            }

            // re-read in case file changed
            matjson::Value doc = matjson::Value::object();
            if (auto res = geode::utils::file::readJson(path); res) {
                doc = res.unwrap();
            }

            // extract accounts entry
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
                accounts.push_back(accountsVal);
            }

            // check duplicate now (race-safe) and overwrite if found
            size_t foundIndex = SIZE_MAX;
            for (size_t i = 0; i < accounts.size(); ++i) {
                if (auto r = accounts[i]["username"].asString();
                    r && r.unwrap() == username) {
                    foundIndex = i;
                    break;
                }
            }

            if (foundIndex != SIZE_MAX) {
                // overwrite existing
                accounts[foundIndex]["gjp2"] = gjp2;

                matjson::Value root = matjson::Value::object();
                root["accounts"] = accounts;
                if (auto r = utils::file::writeToJson(path, root); !r) {
                    Notification::create("Failed to save accounts file.",
                        NotificationIcon::Error)
                        ->show();
                    log::warn("Failed to write accounts file");
                } else {
                    // verify write by re-reading file and checking gjp2
                    matjson::Value verifyDoc = matjson::Value::object();
                    bool verified = false;
                    if (auto vr = geode::utils::file::readJson(path); vr) {
                        verifyDoc = vr.unwrap();
                        matjson::Value vAccounts;
                        if (verifyDoc.contains("accounts")) {
                            vAccounts = verifyDoc["accounts"];
                        } else {
                            vAccounts = verifyDoc;
                        }

                        if (vAccounts.isArray()) {
                            if (auto ar = vAccounts.as<std::vector<matjson::Value>>(); ar) {
                                for (auto const& a : ar.unwrap()) {
                                    if (auto s = a["username"].asString();
                                        s && s.unwrap() == username) {
                                        if (auto g = a["gjp2"].asString();
                                            g && g.unwrap() == gjp2) {
                                            verified = true;
                                        }
                                        break;
                                    }
                                }
                            }
                        } else if (vAccounts.isObject()) {
                            if (auto s = vAccounts["username"].asString();
                                s && s.unwrap() == username) {
                                if (auto g = vAccounts["gjp2"].asString();
                                    g && g.unwrap() == gjp2) {
                                    verified = true;
                                }
                            }
                        }
                    }

                    if (verified) {
                        // reload full list to keep UI in sync with file
                        this->reloadAccountRows();
                        Notification::create(std::string("Overwrote account ") +
                                                 std::string(username.c_str()),
                            NotificationIcon::Success)
                            ->show();
                        log::info("Overwrote account {}", username);
                    } else {
                        Notification::create(
                            "Account overwrite failed: verification mismatch.",
                            NotificationIcon::Error)
                            ->show();
                        log::warn("Account {} not updated after overwrite verification",
                            username);
                    }
                }
                return;
            }

            // append new account if not found
            matjson::Value newAcc = matjson::Value::object();
            newAcc["username"] = username;
            newAcc["gjp2"] = gjp2;
            accounts.push_back(newAcc);

            matjson::Value root = matjson::Value::object();
            root["accounts"] = accounts;
            if (auto r = utils::file::writeToJson(path, root); !r) {
                Notification::create("Failed to save accounts file.",
                    NotificationIcon::Error)
                    ->show();
                log::warn("Failed to write accounts file");
            } else {
                // verify write by re-reading file
                matjson::Value verifyDoc = matjson::Value::object();
                bool found = false;
                if (auto vr = geode::utils::file::readJson(path); vr) {
                    verifyDoc = vr.unwrap();
                    matjson::Value vAccounts;
                    if (verifyDoc.contains("accounts")) {
                        vAccounts = verifyDoc["accounts"];
                    } else {
                        vAccounts = verifyDoc;
                    }

                    if (vAccounts.isArray()) {
                        if (auto ar = vAccounts.as<std::vector<matjson::Value>>(); ar) {
                            for (auto const& a : ar.unwrap()) {
                                if (auto s = a["username"].asString();
                                    s && s.unwrap() == username) {
                                    found = true;
                                    break;
                                }
                            }
                        }
                    } else if (vAccounts.isObject()) {
                        if (auto s = vAccounts["username"].asString();
                            s && s.unwrap() == username) {
                            found = true;
                        }
                    }
                }

                if (found) {
                    log::info("Added account {}", username);
                    // reload list to include the new account
                    this->reloadAccountRows();
                    Notification::create(std::string("Added account ") +
                                             std::string(username.c_str()),
                        NotificationIcon::Success)
                        ->show();
                } else {
                    Notification::create("Account add failed: verification mismatch.",
                        NotificationIcon::Error)
                        ->show();
                    log::warn("Account {} not found after write verification",
                        username);
                }
            }
        });
}

void SwitchAccountPopup::onAccountNodeSelect(AccountNode* node) {
    if (!node || node == m_currentAccountNode)
        return;

    if (m_currentAccountNode) {
        m_currentAccountNode->setCurrent(false);
    }

    node->setCurrent(true);
    m_currentAccountNode = node;

    auto username = node->getUsername();
    auto gjp2 = node->getGjp2();

    std::string msg =
        std::string("Are you sure you want to switch to account '<cg>") +
        username +
        "</c>'?"
        "\n<cy>This will log out your current account, delete account data on "
        "this device and log in to the selected account.</c>"
        "\n<cr>Be sure to save your current account's data to cloud before "
        "switching!</c>";
    createQuickPopup(
        "Switch Account", msg, "No", "Switch", [this, username, gjp2](FLAlertLayer*, bool yes) {
            if (!yes)
                return;

            log::info("switching to account {}", username);
            account::isSwitchingAccount = true;

            auto gjam = GJAccountManager::sharedState();
            account::originalUsername = gjam->m_username;
            account::originalGJP2 = gjam->m_GJP2;
            account::originalAccountID = gjam->m_accountID;
            account::originalUserID =
                GameLevelManager::sharedState()->userIDForAccountID(
                    gjam->m_accountID);
            gjam->unlinkFromAccount();

            account::pendingUsername = username;
            account::pendingGJP2 = gjp2;
            gjam->loginAccount(username, gjp2);
        });
}

void SwitchAccountPopup::onAccountNodeDelete(AccountNode* node) {
    if (!node)
        return;

    auto username = node->getUsername();

    std::string msg =
        std::string("Are you sure you want to remove account '<cg>") +
        username +
        "</c>'?\n<cy>This will remove it from the accounts list.</c>";
    createQuickPopup(
        "Remove Account", msg, "No", "Remove", [this, username](FLAlertLayer*, bool yes) {
            if (!yes)
                return;

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
                    if (auto s = accountsVal["username"].asString();
                        s && s.unwrap() == username) {
                        accounts.clear();
                    }
                }
            }

            bool removed = false;
            for (auto it = accounts.begin(); it != accounts.end(); ++it) {
                if (auto s = (*it)["username"].asString();
                    s && s.unwrap() == username) {
                    accounts.erase(it);
                    removed = true;
                    break;
                }
            }

            if (!removed && accounts.empty()) {
                removed = true;
            }

            if (!removed) {
                Notification::create("Account not found in file.", NotificationIcon::Error)->show();
                return;
            }

            matjson::Value root = matjson::Value::object();
            root["accounts"] = accounts;
            if (auto r = geode::utils::file::writeToJson(path, root); !r) {
                Notification::create("Failed to save accounts file.", NotificationIcon::Error)->show();
                log::warn("Failed to write accounts file");
                return;
            }

            this->reloadAccountRows();
            Notification::create(std::string("Removed account ") + username, NotificationIcon::Success)->show();
            log::info("Removed account {}", username);
        });
}
