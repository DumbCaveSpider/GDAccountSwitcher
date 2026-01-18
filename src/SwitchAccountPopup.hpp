#pragma once

#include <Geode/Geode.hpp>
#include <vector>

using namespace geode::prelude;

class CCMenuItemSpriteExtra;
class CCMenuItemToggler;

class SwitchAccountPopup : public geode::Popup<> {
     public:
      static SwitchAccountPopup* create();

     protected:
      bool setup() override;

     private:
      void onAdd(CCObject* sender);
      void onSelect(CCObject* sender);
      void addAccountRow(const std::string& username, const std::string& gjp2, bool isCurrent = false);

      // per-row select buttons
      std::vector<CCMenuItemSpriteExtra*> m_selectButtons;
      std::vector<std::string> m_usernames;
      std::vector<std::string> m_gjp2s;

      ScrollLayer* m_listLayer = nullptr;
      size_t m_currentAccountIndex = SIZE_MAX;

};