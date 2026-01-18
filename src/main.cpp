#include <Geode/Geode.hpp>
#include <Geode/modify/AccountHelpLayer.hpp>

#include "SwitchAccountPopup.hpp"

using namespace geode::prelude;

class $modify(AWAccountHelpLayer, AccountHelpLayer) {
      void customSetup() {
            AccountHelpLayer::customSetup();

            auto btnSpr = ButtonSprite::create("Change Account", 162.f, true, "goldFont.fnt", "GJ_button_01.png", .0f, 1.f);
            auto btn = CCMenuItemSpriteExtra::create(
                btnSpr,
                this,
                menu_selector(AWAccountHelpLayer::onChangeAccount));

            // i swear why does m_mainLayer->getChildbyID("hide-dropdown-menu") didnt apply at all wtf
            // im screaming in angornyy
            // ig this works fine tho
            CCMenu* menu = nullptr;
            if (auto children = m_mainLayer->getChildren()) {
                  CCObject* iterationOnASingleCCMenuThatShouldveBeenEasyToGet = nullptr;
                  CCARRAY_FOREACH(children, iterationOnASingleCCMenuThatShouldveBeenEasyToGet) {
                        CCNode* node = static_cast<CCNode*>(iterationOnASingleCCMenuThatShouldveBeenEasyToGet);
                        if (auto m = typeinfo_cast<CCMenu*>(node)) {
                              menu = m;
                              break;
                        }
                  }
            }

            if (menu) {
                  menu->addChild(btn);
                  menu->setPositionY(menu->getPositionY() + 20);
                  if (CCMenuItem* backBtn = static_cast<CCMenuItem*>(menu->getChildByIndex(0)))  // this is terrible idea but ik that back button is always index 0 (i hope........)
                        backBtn->setPositionY(backBtn->getPositionY() - 20);
                  btn->setPosition({m_unlinkAccountButton->getPositionX(), m_unlinkAccountButton->getPositionY() - 45});
                  return;
            } else {
                  // just as a backup menu if ts doesnt work
                  CCMenu* menuNew = CCMenu::create();
                  m_mainLayer->addChild(menuNew);
                  menuNew->setPosition({0, 0});
                  menuNew->addChild(btn);
                  btn->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height - 265});
            }
      }

      void onChangeAccount(CCObject* sender) {
            // first time?
            if (!Mod::get()->getSavedValue<bool>("firstLaunch")) {
                  FLAlertLayer::create(
                      "Important Notice",
                      "This mod will required to store your <cr>GJP2 (Your GD Account Password)</c> locally in order to switch accounts.\n<cy>If you wish to not use your GJP2 in any purpose, don't use this mod!</c>",
                      "OK")
                      ->show();
                  Mod::get()->setSavedValue<bool>("firstLaunch", true);
                  return;
            }
            // open popup yes
            auto popup = SwitchAccountPopup::create();
            popup->show();
      };
};