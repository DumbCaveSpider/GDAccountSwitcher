#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class SwitchLoadingLayer : public CCBlockLayer {
public:
    static SwitchLoadingLayer* create();

    void setStatus(std::string status);

    void show();
    void dismiss();

protected:
    bool init() override;
    void keyBackClicked() override {}

    Label* m_statusLabel = nullptr;
};
