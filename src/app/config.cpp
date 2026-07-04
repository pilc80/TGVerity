#include "app/config.h"

#include <cstdlib>
#include <iostream>

namespace tgverity {
namespace {

std::string envOrEmpty(const char* key) {
    if (const auto value = std::getenv(key)) {
        return value;
    }
    return {};
}

} // namespace

bool AppConfig::hasTelegramCredentials() const {
    return !apiId.empty() && !apiHash.empty();
}

AppConfig AppConfig::fromEnvironment() {
    auto config = AppConfig{
        .apiId = envOrEmpty("TGVERITY_API_ID"),
        .apiHash = envOrEmpty("TGVERITY_API_HASH"),
        .phone = envOrEmpty("TGVERITY_PHONE"),
        .tdlibDir = envOrEmpty("TGVERITY_TDLIB_DIR"),
    };
    if (config.tdlibDir.empty()) {
        config.tdlibDir = ".tdlib";
    }
    return config;
}

AppConfig AppConfig::fromEnvironmentOrPrompt() {
    auto config = fromEnvironment();
    if (config.apiId.empty()) {
        std::cout << "Telegram API ID: ";
        std::getline(std::cin, config.apiId);
    }
    if (config.apiHash.empty()) {
        std::cout << "Telegram API hash: ";
        std::getline(std::cin, config.apiHash);
    }
    if (config.phone.empty()) {
        std::cout << "Phone: ";
        std::getline(std::cin, config.phone);
    }
    return config;
}

} // namespace tgverity
