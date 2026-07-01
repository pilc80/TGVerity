#pragma once

#include "tgverity/relay_packet.h"

#include <optional>
#include <string>

namespace tgverity {

struct OutboundRelayText {
    std::string text;
};

struct InboundRelayPacket {
    RelayPacket packet;
};

class Bridge {
public:
    [[nodiscard]] OutboundRelayText buildRelayTelegramText(const std::string& plaintext) const;
    [[nodiscard]] std::optional<InboundRelayPacket> tryParseTelegramText(const std::string& text) const;
};

} // namespace tgverity
