#include "SwitchLoadingLayer.hpp"

#include <cue/LoadingCircle.hpp>

SwitchLoadingLayer* SwitchLoadingLayer::create() {
    auto ret = new SwitchLoadingLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool SwitchLoadingLayer::init() {
    if (!CCBlockLayer::init()) return false;

    this->setID("switch-loading-layer"_spr);
    this->setColor({0, 0, 0});
    this->setOpacity(150);
    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);

    auto winSize = CCDirector::get()->getWinSize();

    auto circle = cue::LoadingCircle::create(true);
    circle->addToLayer(this);  // centered on the layer (full screen)

    m_statusLabel = Label::create("", "bigFont.fnt");
    m_statusLabel->setID("status-label");
    m_statusLabel->setLimitLabelWidth(winSize.width - 40.f, .5f, .2f);
    m_statusLabel->setPosition({winSize.width / 2, winSize.height / 2 - 55.f});
    this->addChild(m_statusLabel);

    return true;
}

void SwitchLoadingLayer::setStatus(std::string status) {
    if (m_statusLabel) m_statusLabel->setText(std::move(status));
}

void SwitchLoadingLayer::show() {
    auto scene = CCScene::get();
    if (!scene) return;
    scene->addChild(this, std::max(1000, scene->getHighestChildZ() + 1));
}

void SwitchLoadingLayer::dismiss() {
    this->removeFromParent();
}
