#include "app/config.h"

#include <cstdlib>

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

} // namespace tgverity
