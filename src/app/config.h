#pragma once

#include <string>

namespace tgverity {

struct AppConfig {
    std::string apiId;
    std::string apiHash;
    std::string phone;
    std::string tdlibDir;

    [[nodiscard]] bool hasTelegramCredentials() const;
    [[nodiscard]] static AppConfig fromEnvironment();
    [[nodiscard]] static AppConfig fromEnvironmentOrPrompt();
};

} // namespace tgverity
