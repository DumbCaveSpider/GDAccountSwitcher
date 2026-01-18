#include <Geode/Geode.hpp>
#include <Geode/modify/GJAccountManager.hpp>

using namespace geode::prelude;
using namespace geode::utils;

class $modify(AWGJAccountManager, GJAccountManager) {
     public:
      void onLoginAccountCompleted(gd::string response, gd::string tag) {
            GJAccountManager::onLoginAccountCompleted(response, tag);

            // for debugging
            log::debug("raw response: {} {}", response, tag);

            bool linked = false;
            std::string s = response;

            geode::utils::string::trim(s);

            auto pos = s.find(',');
            if (pos != std::string::npos) {
                  std::string strAccountID = s.substr(0, pos);
                  std::string strUserID = s.substr(pos + 1);
                  geode::utils::string::trim(strAccountID);
                  geode::utils::string::trim(strUserID);
                  int accountID = geode::utils::numFromString<int>(strAccountID).unwrapOr(0);
                  int userID = geode::utils::numFromString<int>(strUserID).unwrapOr(0);
                  this->sharedState()->linkToAccount(this->m_username, this->m_GJP2, accountID, userID);
                  log::debug("linkToAccount called with accountID={} userID={}", accountID, userID);
                  linked = true;
            } else {
                  log::warn("Failed to parse login response '{}'", response);
            }

            if (!linked) {
                  log::debug("no account/user IDs found to link");
            }
      }
};
