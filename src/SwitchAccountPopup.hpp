#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class SwitchAccountPopup : public geode::Popup<> {
     public:
      static SwitchAccountPopup* create();

     protected:
      bool setup() override;

     private:
};