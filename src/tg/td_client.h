#pragma once

#include "app/config.h"

#include <cstdint>
#include <optional>
#include <string>

namespace tgverity {

class TdClient {
public:
    TdClient();
    ~TdClient();

    TdClient(const TdClient&) = delete;
    TdClient& operator=(const TdClient&) = delete;

    [[nodiscard]] static std::string version();

    void send(const std::string& json);
    [[nodiscard]] std::optional<std::string> receive(double timeoutSeconds);

    int login(const AppConfig& config);
    int chats(const AppConfig& config);
    int resolve(const AppConfig& config, const std::string& username);
    int sendText(const AppConfig& config, std::int64_t chatId, const std::string& text);
    int watch(const AppConfig& config, std::optional<std::int64_t> chatId);

private:
    [[nodiscard]] int ensureReady(const AppConfig& config, bool allowInteractiveAuth);

    void* _client = nullptr;
};

} // namespace tgverity
