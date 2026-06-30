#include "tgverity/tg_client.h"

#include <stdexcept>

namespace tgverity {

class TdlibTelegramClient final : public TelegramClient {
public:
    void sendRelayPacketText(const std::string& chatId, const std::string& packetText) override {
        static_cast<void>(chatId);
        static_cast<void>(packetText);
        throw std::runtime_error("TDLib integration is not implemented yet");
    }
};

} // namespace tgverity
