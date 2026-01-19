#include <Geode/Geode.hpp>
#include <Geode/modify/GJAccountManager.hpp>
#include <matjson.hpp>
#include "../PendingSwitch.hpp"

using namespace geode::prelude;
using namespace geode::utils;

class $modify(AWGJAccountManager, GJAccountManager) {
     public:
      void onLoginAccountCompleted(gd::string response, gd::string tag) {
            GJAccountManager::onLoginAccountCompleted(response, tag);

            // for debugging
            log::debug("login complete response: {}", response);

            bool linked = false;
            std::string s = response;
            const char* tagChar = tag.c_str();

            geode::utils::string::trim(s);

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

                  std::string chosenUsername = "";
                  std::string chosenGJP2 = "";

                  //pending switch request
                  if (!account_switcher::pendingUsername.empty()) {
                        chosenUsername = account_switcher::pendingUsername;
                        if (!account_switcher::pendingGJP2.empty()) {
                              chosenGJP2 = account_switcher::pendingGJP2;
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
                        account_switcher::pendingUsername.clear();
                        account_switcher::pendingGJP2.clear();
                  }

                  // revert to the original account if still empty
                  if (chosenUsername.empty()) {
                        chosenUsername = this->m_username;
                        chosenGJP2 = this->m_GJP2;
                        log::warn("Account invaild for switch, reverting to current account {}", chosenUsername);
                        Notification::create("Account Switch failed, reverting to " + chosenUsername + ".", NotificationIcon::Error)->show();
                  }

                  this->addDLToActive(tagChar);
                  this->linkToAccount(chosenUsername, chosenGJP2, accountID, userID);
                  log::debug("linkToAccount called with username={} accountID={} userID={}", chosenUsername, accountID, userID);
                  linked = true;
            } else {
                  log::warn("Failed to parse login response '{}'", response);
            }

            if (!linked) {
                  log::debug("no account/user IDs found to link");
            }
      }
};
