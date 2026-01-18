#include "SwitchAccountPopup.hpp"

#include <Geode/Geode.hpp>

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
      if (!SwitchAccountPopup::setup()) return false;
      setTitle("Switch Account");
      m_noElasticity = true;
      return true;
}