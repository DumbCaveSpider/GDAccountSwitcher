#pragma once

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include <vector>
#include "AccountNode.hpp"

using namespace geode::prelude;
class SwitchAccountPopup : public geode::Popup {
public:
    static SwitchAccountPopup* create();

protected:
    bool init() override;

private:
    void onAdd(CCObject* sender);
    void onInfo(CCObject* sender);
    void onShowSavedFolder(CCObject* sender);

    void onAccountNodeSelect(AccountNode* node);
    void onAccountNodeDelete(AccountNode* node);
    void addAccountRow(const std::string& username, const std::string& gjp2, bool isCurrent = false);

    // per-row select buttons
    std::vector<AccountNode*> m_accountNodes;

    cue::ListNode* m_listLayer = nullptr;
    AccountNode* m_currentAccountNode = nullptr;

    void clearAccountRows();
    void reloadAccountRows();
    void updateListLayerSize();
};