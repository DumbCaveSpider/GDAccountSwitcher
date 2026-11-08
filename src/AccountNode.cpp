#include "AccountNode.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

AccountNode* AccountNode::create(const std::string& username,
    const std::string& gjp2,
    bool isCurrent,
    Callback onDelete,
    Callback onSelect) {
    auto ret = new AccountNode();
    if (ret && ret->init(username, gjp2, isCurrent, onDelete, onSelect)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool AccountNode::init(const std::string& username,
    const std::string& gjp2,
    bool isCurrent,
    Callback onDelete,
    Callback onSelect) {
    if (!CCLayer::init())
        return false;

    m_username = username;
    m_gjp2 = gjp2;
    m_onDelete = std::move(onDelete);
    m_onSelect = std::move(onSelect);

    setContentSize({350.f, 50.f});
    setAnchorPoint({0, 0});

    auto label = Label::create(username, "goldFont.fnt");
    label->setAnchorPoint({0.0f, 0.5f});
    label->setScale(0.9f);
    label->setPosition({15.f, 25.f});
    addChild(label);

    auto const centerY = getContentSize().height / 2;

    auto delBtn = Button::createWithSpriteFrameName("GJ_deleteSongBtn_001.png", [this](Button*) {
        if (m_onDelete) m_onDelete(this);
    });
    delBtn->setPosition({getContentSize().width - 80.f, centerY});
    addChild(delBtn);

    m_selectButton = Button::createWithSpriteFrameName("GJ_selectSongBtn_001.png", [this](Button*) {
        if (m_onSelect) m_onSelect(this);
    });
    m_selectButton->setPosition({getContentSize().width - 30.f, centerY});
    addChild(m_selectButton);

    // rows live inside a scroll list, so let drags that start on a button scroll the list
    for (auto btn : {delBtn, m_selectButton}) {
        btn->setSwallowTouches(false);
        btn->setDragCancelDistance(5.f);
    }

    setCurrent(isCurrent);

    return true;
}

void AccountNode::setCurrent(bool isCurrent) {
    m_isCurrent = isCurrent;
    if (!m_selectButton)
        return;

    if (isCurrent) {
        m_selectButton->setDisplayNode(CCSprite::createWithSpriteFrameName("GJ_selectSongOnBtn_001.png"));
        m_selectButton->setEnabled(false);
    } else {
        m_selectButton->setDisplayNode(CCSprite::createWithSpriteFrameName("GJ_selectSongBtn_001.png"));
        m_selectButton->setEnabled(true);
    }
}
