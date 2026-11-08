#include <Geode/Geode.hpp>
#include <Geode/modify/AccountHelpLayer.hpp>

#include "../SwitchAccountPopup.hpp"

using namespace geode::prelude;

class $modify(AWAccountHelpLayer, AccountHelpLayer) {
    void customSetup() {
        AccountHelpLayer::customSetup();

        auto menu = m_buttonMenu;
        if (!menu || !m_unlinkAccountButton || !m_refreshLoginButton) {
            log::debug("buttons not found, skipping Switch Account button");
            return;
        }

        constexpr float spacing = 45.f;
        constexpr float shiftUp = 20.f;  // make room for a 4th button

        CCNode* manageBtn = nullptr;
        if (auto children = menu->getChildren()) {
            auto idx = children->indexOfObject(m_unlinkAccountButton);
            if (idx != CC_INVALID_INDEX && idx > 0) {
                manageBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(children->objectAtIndex(idx - 1));
            }
        }

        for (CCNode* node : {static_cast<CCNode*>(m_refreshLoginButton), manageBtn, static_cast<CCNode*>(m_unlinkAccountButton)}) {
            if (node) node->setPositionY(node->getPositionY() + shiftUp);
        }

        auto parent = menu->getParent() ? menu->getParent() : m_mainLayer;
        auto targetInMenu = m_unlinkAccountButton->getPosition() - CCPoint(0.f, spacing);
        auto targetPos = parent->convertToNodeSpace(menu->convertToWorldSpace(targetInMenu));

        auto btn = Button::createWithNode(
            ButtonSprite::create("Switch Account", 160.f, true, "goldFont.fnt", "GJ_button_01.png", .0f, 1.f),
            [this](Button*) { this->onChangeAccount(); });
        btn->setID("switch-account-button"_spr);
        btn->setPosition(targetPos);
        btn->setZOrder(menu->getZOrder());
        parent->addChild(btn);
    }

    void onChangeAccount() {
        // first time?
        if (!Mod::get()->getSavedValue<bool>("firstLaunch")) {
            FLAlertLayer::create(
                "Important Notice",
                "This mod will required to store your <cr>GJP2 (Your Encrypted GD Account Password)</c> locally in order to switch accounts.\n<cy>If you wish to not use your GJP2 in any purpose, don't use this mod!</c>",
                "OK")
                ->show();
            Mod::get()->setSavedValue<bool>("firstLaunch", true);
            return;
        }
        // open popup yes
        if (auto popup = SwitchAccountPopup::create()) popup->show();
    }
};
