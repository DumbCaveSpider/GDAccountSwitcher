#pragma once

#include <functional>
#include <string>

namespace account {
    using StatusCallback = std::function<void(std::string const& status)>;
    using FinishCallback = std::function<void(bool success)>;

    struct SwitchOptions {
        bool skipBackup = false;
        bool skipCloudLoad = false;
    };

    bool isSwitching();
    bool startSwitch(std::string username, std::string gjp2, SwitchOptions options, StatusCallback onStatus,
        FinishCallback onFinished);
    void handleLoginResponse(std::string response);
}
