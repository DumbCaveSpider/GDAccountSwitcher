#include <Geode/Geode.hpp>
#include <Geode/modify/GJAccountManager.hpp>

#include "../AccountSwitch.hpp"

using namespace geode::prelude;

class $modify(AWGJAccountManager, GJAccountManager) {
    void onLoginAccountCompleted(std::string response, std::string tag) {
        GJAccountManager::onLoginAccountCompleted(response, tag);
        account::handleLoginResponse(std::move(response));
    }
};
