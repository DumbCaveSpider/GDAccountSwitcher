#pragma once
#include <string>

namespace account {
    inline std::string pendingUsername;
    inline std::string pendingGJP2;

    inline std::string originalUsername;
    inline std::string originalGJP2;
    inline int originalAccountID = 0;
    inline int originalUserID = 0;

    inline bool isSwitchingAccount = false;
}
