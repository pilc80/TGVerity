#pragma once

#include <string>

namespace tgverity {

class TelegramClient {
public:
    virtual ~TelegramClient() = default;
    virtual void sendRelayPacketText(const std::string& chatId, const std::string& packetText) = 0;
};

class StubTelegramClient final : public TelegramClient {
public:
    void sendRelayPacketText(const std::string& chatId, const std::string& packetText) override;
};

} // namespace tgverity
