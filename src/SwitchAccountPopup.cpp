#include "SwitchAccountPopup.hpp"

#include <Geode/Geode.hpp>
#include <functional>
#include <matjson.hpp>
#include "PendingSwitch.hpp"

using namespace geode::prelude;

SwitchAccountPopup* SwitchAccountPopup::create() {
      auto ret = new SwitchAccountPopup();
      if (ret && ret->initAnchored(400.f, 300.f)) {
            ret->autorelease();
            return ret;
      }
      CC_SAFE_DELETE(ret);
      return nullptr;
}

bool SwitchAccountPopup::setup() {
      setTitle("Switch Online Account");
      m_noElasticity = true;
      // remove close button
      m_closeBtn->removeFromParent();

      // create and add GJListLayer centered in m_mainLayer
      auto listLayer = ScrollLayer::create({350.f, 200.f});
      listLayer->setAnchorPoint({.0f, .0f});
      listLayer->setPosition({25, 60});

      if (listLayer->m_contentLayer) {
            listLayer->m_contentLayer->setLayout(
                ColumnLayout::create()
                    ->setAxisAlignment(AxisAlignment::End)
                    ->setGap(1.f)
                    ->setAxisReverse(true)
                    ->setAutoGrowAxis(200.f));
      }

      m_mainLayer->addChild(listLayer);

      // add list border
      addListBorders(m_mainLayer, {m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height / 2 + 10}, {350.f, 200.f});

      // load accounts from file and populate the content layer
      auto path = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID() / "accounts.json";

      matjson::Value doc = matjson::Value::object();
      if (auto res = geode::utils::file::readJson(path); res) {
            doc = res.unwrap();
      } else {
            log::info("No accounts file found or failed parsing, continuing with empty list");
      }

      // extract accounts entry: support either an object or an array
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
            } else {
                  log::warn("accounts value was array but failed to convert to vector");
            }
      } else if (accountsVal.isObject()) {
            // only treat as a single account object if it contains a username
            if (accountsVal.contains("username")) {
                  accounts.push_back(accountsVal);
            }
      }

      m_listLayer = listLayer;
      std::string currentUsername = GJAccountManager::sharedState()->m_username;

      for (auto const& acc : accounts) {
            std::string uname = "";
            if (auto r = acc["username"].asString(); r) uname = r.unwrap();
            std::string gjp2 = "";
            if (auto r = acc["gjp2"].asString(); r) gjp2 = r.unwrap();

            this->addAccountRow(uname, gjp2, uname == currentUsername);
      }

      // add button
      auto addBtn = CCMenuItemSpriteExtra::create(
          ButtonSprite::create("Add", "goldFont.fnt", "GJ_button_01.png"),
          this,
          menu_selector(SwitchAccountPopup::onAdd));
      m_buttonMenu->addChild(addBtn);
      addBtn->setPosition({m_mainLayer->getContentSize().width / 2 + 70, 25.f});

      // cancel button
      auto cancelBtn = CCMenuItemSpriteExtra::create(
          ButtonSprite::create("Cancel", "goldFont.fnt", "GJ_button_06.png"),
          this,
          menu_selector(SwitchAccountPopup::onClose));
      m_buttonMenu->addChild(cancelBtn);
      cancelBtn->setPosition({m_mainLayer->getContentSize().width / 2 - 50, 25.f});

      // info button
      auto infoBtn = CCMenuItemSpriteExtra::create(
          CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"),
          this,
          menu_selector(SwitchAccountPopup::onInfo));
      m_buttonMenu->addChild(infoBtn);
      infoBtn->setPosition({m_mainLayer->getContentSize().width - 20.f, m_mainLayer->getContentSize().height - 20.f});
      return true;
}

void SwitchAccountPopup::onInfo(CCObject* sender) {
      FLAlertLayer::create("How to use", "Login to your <cy>current account</c>, then click <cg>Add</c> to store it locally.\n\nTo switch accounts, simply click the <cg>Select</c> button next to the desired account. The game will <cr>log out your current account</c> and <cg>log back in to the selected account</c> automatically.", "OK")->show();
}

void SwitchAccountPopup::addAccountRow(const std::string& username, const std::string& gjp2, bool toggleDefault) {
      if (!m_listLayer || !m_listLayer->m_contentLayer) return;

      auto row = CCLayer::create();
      row->setAnchorPoint({0, 0});
      row->setContentSize({m_listLayer->m_contentLayer->getContentSize().width, 50.f});

      auto rowColor = CCLayerColor::create({130, 64, 32, 255}, row->getContentSize().width, row->getContentSize().height);
      rowColor->setAnchorPoint({0, 1});
      row->addChild(rowColor, -1);

      auto label = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
      label->setAnchorPoint({0.0f, 0.5f});
      label->setScale(0.9f);
      label->setPosition({15.f, row->getContentSize().height / 2});
      row->addChild(label);

      // create delete button sprite
      auto delSpr = CCSprite::createWithSpriteFrameName("GJ_deleteSongBtn_001.png");
      auto delBtn = CCMenuItemSpriteExtra::create(delSpr, this, menu_selector(SwitchAccountPopup::onDelete));
      auto dmenu = CCMenu::create();
      dmenu->addChild(delBtn);
      dmenu->setPosition({row->getContentSize().width - 80.f, row->getContentSize().height / 2});
      dmenu->setAnchorPoint({0.f, 0.5f});
      row->addChild(dmenu);

      // create select button sprites
      auto normalSpr = CCSprite::createWithSpriteFrameName("GJ_selectSongBtn_001.png");
      auto onSpr = CCSprite::createWithSpriteFrameName("GJ_selectSongOnBtn_001.png");
      auto selectBtn = CCMenuItemSpriteExtra::create(normalSpr, this, menu_selector(SwitchAccountPopup::onSelect));
      auto smenu = CCMenu::create();
      smenu->addChild(selectBtn);
      smenu->setPosition({row->getContentSize().width - 30.f, row->getContentSize().height / 2});
      smenu->setAnchorPoint({0.f, 0.5f});
      row->addChild(smenu);

      // assign index before pushing
      size_t index = m_selectButtons.size();
      m_selectButtons.push_back(selectBtn);
      m_deleteButtons.push_back(delBtn);
      m_usernames.push_back(username);
      m_gjp2s.push_back(gjp2);

      // if this is current account, show as on and disable
      if (toggleDefault) {
            selectBtn->setSprite(onSpr);
            selectBtn->setEnabled(false);
            delBtn->setColor({100, 100, 100});
            delBtn->setEnabled(false);  // don't allow deleting current logged-in account
            m_currentAccountIndex = index;
      } else {
            selectBtn->setSprite(normalSpr);
            selectBtn->setEnabled(true);
            delBtn->setEnabled(true);
      }

      m_listLayer->m_contentLayer->addChild(row);
      m_listLayer->m_contentLayer->updateLayout(row);
}

void SwitchAccountPopup::onAdd(CCObject* sender) {
      // ask for confirmation and append the new account on yes
      auto path = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID() / "accounts.json";

      createQuickPopup("Add Account",
                       "Are you sure you want to add your <cg>current logged in</c> account?\n<cy>This will store your currently login Username and GJP2 (Password) locally on your device.</c>",
                       "No", "Yes", [this, path](auto, bool yes) {
                             if (!yes) return;

                             // fetch current credentials at confirmation time
                             std::string username = GJAccountManager::sharedState()->m_username;
                             std::string gjp2 = GJAccountManager::sharedState()->m_GJP2;

                             if (username.empty() || gjp2.empty()) {
                                   Notification::create("Error when getting current account.", NotificationIcon::Error)->show();
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
                                   if (auto r = accounts[i]["username"].asString(); r && r.unwrap() == username) {
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
                                         Notification::create("Failed to save accounts file.", NotificationIcon::Error)->show();
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
                                                                 if (auto s = a["username"].asString(); s && s.unwrap() == username) {
                                                                       if (auto g = a["gjp2"].asString(); g && g.unwrap() == gjp2) {
                                                                             verified = true;
                                                                       }
                                                                       break;
                                                                 }
                                                           }
                                                     }
                                               } else if (vAccounts.isObject()) {
                                                     if (auto s = vAccounts["username"].asString(); s && s.unwrap() == username) {
                                                           if (auto g = vAccounts["gjp2"].asString(); g && g.unwrap() == gjp2) {
                                                                 verified = true;
                                                           }
                                                     }
                                               }
                                         }

                                         if (verified) {
                                               // update in-memory gjp2 if we have that row loaded
                                               if (foundIndex < m_gjp2s.size()) {
                                                     m_gjp2s[foundIndex] = gjp2;
                                               }
                                               Notification::create(std::string("Overwrote account ") + username, NotificationIcon::Success)->show();
                                               log::info("Overwrote account {}", username);
                                         } else {
                                               Notification::create("Account overwrite failed: verification mismatch.", NotificationIcon::Error)->show();
                                               log::warn("Account {} not updated after overwrite verification", username);
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
                                   Notification::create("Failed to save accounts file.", NotificationIcon::Error)->show();
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
                                                           if (auto s = a["username"].asString(); s && s.unwrap() == username) {
                                                                 found = true;
                                                                 break;
                                                           }
                                                     }
                                               }
                                         } else if (vAccounts.isObject()) {
                                               if (auto s = vAccounts["username"].asString(); s && s.unwrap() == username) {
                                                     found = true;
                                               }
                                         }
                                   }

                                   if (found) {
                                         log::info("Added account {}", username);
                                         // append to UI
                                         this->addAccountRow(username, gjp2, false);
                                         Notification::create(std::string("Added account ") + username, NotificationIcon::Success)->show();
                                   } else {
                                         Notification::create("Account add failed: verification mismatch.", NotificationIcon::Error)->show();
                                         log::warn("Account {} not found after write verification", username);
                                   }
                             }
                       });
}

void SwitchAccountPopup::onSelect(CCObject* sender) {
      auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
      if (!btn) return;

      // find index
      size_t idx = SIZE_MAX;
      for (size_t i = 0; i < m_selectButtons.size(); ++i) {
            if (m_selectButtons[i] == btn) {
                  idx = i;
                  break;
            }
      }
      if (idx == SIZE_MAX) return;

      // if user clicked the current account, do nothing
      if (idx == m_currentAccountIndex) return;

      gd::string username = m_usernames.size() > idx ? m_usernames[idx] : "";
      gd::string gjp2 = m_gjp2s.size() > idx ? m_gjp2s[idx] : "";

      {
            std::string msg = std::string("Are you sure you want to switch to account '<cg>") + username + "</c>'?"
                  "\n<cy>This will log out your current account, delete account data on this device and log in to the selected account.</c>"
                  "\n<cr>Be sure to save your current account's data before switching!</c>";
            createQuickPopup("Switch Account", msg,
                             "No", "Switch", [this, idx, username, gjp2](FLAlertLayer*, bool yes) {
                                   if (!yes) return;

                                   // previous current -> normal and enabled
                                   if (m_currentAccountIndex != SIZE_MAX && m_currentAccountIndex < m_selectButtons.size()) {
                                         auto prevBtn = m_selectButtons[m_currentAccountIndex];
                                         if (prevBtn) {
                                               prevBtn->setSprite(CCSprite::createWithSpriteFrameName("GJ_selectSongBtn_001.png"));
                                               prevBtn->setEnabled(true);
                                         }
                                   }

                                   // set new current to on sprite and disable it
                                   if (idx < m_selectButtons.size()) {
                                         auto newBtn = m_selectButtons[idx];
                                         if (newBtn) {
                                               newBtn->setSprite(CCSprite::createWithSpriteFrameName("GJ_selectSongOnBtn_001.png"));
                                               newBtn->setEnabled(false);
                                         }
                                         m_currentAccountIndex = idx;
                                   }

                                   log::info("switching to account {}", username);

                                   auto gjam = GJAccountManager::sharedState();
                                   // logout current account
                                   gjam->unlinkFromAccount();

                                   // mark pending switch so the login callback
                                   account_switcher::pendingUsername = username;
                                   account_switcher::pendingGJP2 = gjp2;

                                   gjam->loginAccount(username, gjp2);
                                   log::debug("{}: {} {} {}", username, gjp2, gjam->m_accountID, GameLevelManager::sharedState()->userIDForAccountID(gjam->m_accountID));
                                   Notification::create(std::string("Switched to ") + username, NotificationIcon::Success)->show();
                                   
                             });
      }
}

void SwitchAccountPopup::onDelete(CCObject* sender) {
      auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
      if (!btn) return;

      // find index
      size_t idx = SIZE_MAX;
      for (size_t i = 0; i < m_deleteButtons.size(); ++i) {
            if (m_deleteButtons[i] == btn) {
                  idx = i;
                  break;
            }
      }
      if (idx == SIZE_MAX) return;

      gd::string username = m_usernames.size() > idx ? m_usernames[idx] : "";

      std::string msg = std::string("Are you sure you want to delete account '<cg>") + username + "</c>'?\n<cy>This will remove it from local storage.</c>";
      createQuickPopup("Delete Account", msg, "No", "Delete", [this, idx, username, btn](FLAlertLayer*, bool yes) {
            if (!yes) return;

            // remove from file
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
                        if (auto s = accountsVal["username"].asString(); s && s.unwrap() == username) {
                              // clear to empty array
                              accounts.clear();
                        }
                  }
            }

            bool removed = false;
            for (auto it = accounts.begin(); it != accounts.end(); ++it) {
                  if (auto s = (*it)["username"].asString(); s && s.unwrap() == username) {
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

            // remove from UI
            CCNode* menu = btn->getParent();
            CCNode* row = menu ? menu->getParent() : nullptr;
            if (row && m_listLayer && m_listLayer->m_contentLayer) {
                  row->removeFromParent();
                  m_listLayer->m_contentLayer->updateLayout(true);
            }

            // remove from memory
            if (idx < m_usernames.size()) m_usernames.erase(m_usernames.begin() + idx);
            if (idx < m_gjp2s.size()) m_gjp2s.erase(m_gjp2s.begin() + idx);
            if (idx < m_selectButtons.size()) m_selectButtons.erase(m_selectButtons.begin() + idx);
            if (idx < m_deleteButtons.size()) m_deleteButtons.erase(m_deleteButtons.begin() + idx);

            if (m_currentAccountIndex == idx) {
                  m_currentAccountIndex = SIZE_MAX;
            } else if (m_currentAccountIndex > idx) {
                  --m_currentAccountIndex;
            }

            Notification::create(std::string("Deleted account ") + username, NotificationIcon::Success)->show();
            log::info("Deleted account {}", username);
      });
}
