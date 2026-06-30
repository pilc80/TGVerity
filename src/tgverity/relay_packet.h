#pragma once

#include <optional>
#include <string>

namespace tgverity {

struct RelayPacket {
    std::string version;
    std::string type;
    std::string packetId;
    std::string body;

    static RelayPacket createText(std::string body);
    std::string toTelegramText() const;
    static std::optional<RelayPacket> fromTelegramText(const std::string& text);
};

bool isRelayPacketText(const std::string& text);

} // namespace tgverity
