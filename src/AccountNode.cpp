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

    auto label = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
    label->setAnchorPoint({0.0f, 0.5f});
    label->setScale(0.9f);
    label->setPosition({15.f, 25.f});
    addChild(label);

    auto delSpr = CCSprite::createWithSpriteFrameName("GJ_deleteSongBtn_001.png");
    auto delBtn = CCMenuItemSpriteExtra::create(delSpr, this, menu_selector(AccountNode::onDeletePressed));

    auto normalSpr = CCSprite::createWithSpriteFrameName("GJ_selectSongBtn_001.png");
    auto onSpr = CCSprite::createWithSpriteFrameName("GJ_selectSongOnBtn_001.png");
    m_selectButton = CCMenuItemSpriteExtra::create(normalSpr, this, menu_selector(AccountNode::onSelectPressed));

    auto rowMenu = CCMenu::create();
    rowMenu->addChild(delBtn);
    rowMenu->addChild(m_selectButton);
    rowMenu->setPosition({getContentSize().width - 80.f, getContentSize().height / 2});
    rowMenu->setAnchorPoint({0.f, 0.5f});
    delBtn->setPosition({0.f, 0.f});
    m_selectButton->setPosition({50.f, 0.f});
    addChild(rowMenu);

    setCurrent(isCurrent);

    return true;
}

void AccountNode::setCurrent(bool isCurrent) {
    m_isCurrent = isCurrent;
    if (!m_selectButton)
        return;

    if (isCurrent) {
        m_selectButton->setSprite(CCSprite::createWithSpriteFrameName("GJ_selectSongOnBtn_001.png"));
        m_selectButton->setEnabled(false);
    } else {
        m_selectButton->setSprite(CCSprite::createWithSpriteFrameName("GJ_selectSongBtn_001.png"));
        m_selectButton->setEnabled(true);
    }
}

void AccountNode::onDeletePressed(CCObject*) {
    if (m_onDelete)
        m_onDelete(this);
}

void AccountNode::onSelectPressed(CCObject*) {
    if (m_onSelect)
        m_onSelect(this);
}
