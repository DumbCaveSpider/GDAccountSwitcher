#include <Geode/Geode.hpp>
#include <Geode/modify/AccountHelpLayer.hpp>

using namespace geode::prelude;

class $modify(AWAccountHelpLayer, AccountHelpLayer) {
    void customSetup() {
        // add change account buttonsprites
        auto btnSpr = ButtonSprite::create("Change Account", 176, false, "goldFont.fnt", "GJ_button_01.png", 0, 1.0f);
        auto btn = CCMenuItemSpriteExtra::create(
            btnSpr,
            this,
            menu_selector(AWAccountHelpLayer::onChangeAccount)
        );
        if (auto menu = this->getChildByID("hide-dropdown-menu")) {
            menu->addChild(btn);
            btn->setPosition({m_unlinkAccountButton->getPositionX(), m_unlinkAccountButton->getPositionY() - 40});
        }
        AWAccountHelpLayer::customSetup();
    }

    void onChangeAccount(CCObject* sender) {
        log::debug("Change Account button clicked");
    }
};