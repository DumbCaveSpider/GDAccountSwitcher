#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class AccountNode : public cocos2d::CCLayer {
public:
    using Callback = std::function<void(AccountNode*)>;

    static AccountNode* create(const std::string& username,
        const std::string& gjp2,
        bool isCurrent,
        Callback onDelete,
        Callback onSelect);

    const std::string& getUsername() const { return m_username; }
    const std::string& getGjp2() const { return m_gjp2; }

    void setCurrent(bool isCurrent);

private:
    bool init(const std::string& username,
        const std::string& gjp2,
        bool isCurrent,
        Callback onDelete,
        Callback onSelect);

    std::string m_username;
    std::string m_gjp2;
    bool m_isCurrent = false;
    Callback m_onDelete;
    Callback m_onSelect;
    Button* m_selectButton = nullptr;
};
