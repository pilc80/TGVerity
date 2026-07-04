#pragma once

#include "tgverity/crypto_provider.h"

#include <cstdint>
#include <optional>
#include <string>

namespace tgverity {

// v1 TGVerity relay packet carried as Telegram-visible text.
struct RelayPacket {
    std::uint16_t cryptoSuite = 0;   // CryptoSuite id (0 = Identity, insecure)
    std::string type;                // "relay_text" | "relay_ack"
    std::string packetId;            // session-scoped id
    std::string refId = "-";         // ACK: referenced packet_id; "-" otherwise
    Bytes body;                      // crypto provider output (empty for ACK)

    static RelayPacket createText(Bytes body, std::string packetId);
    static RelayPacket createAck(std::string refId, std::string packetId);
    std::string toTelegramText() const;
    static std::optional<RelayPacket> fromTelegramText(const std::string& text);
};

bool isRelayPacketText(const std::string& text);

} // namespace tgverity
