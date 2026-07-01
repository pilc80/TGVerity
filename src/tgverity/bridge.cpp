#include "tgverity/bridge.h"

namespace tgverity {

OutboundRelayText Bridge::buildRelayTelegramText(const std::string& plaintext) const {
    return {.text = RelayPacket::createText(plaintext).toTelegramText()};
}

std::optional<InboundRelayPacket> Bridge::tryParseTelegramText(const std::string& text) const {
    auto packet = RelayPacket::fromTelegramText(text);
    if (!packet) {
        return std::nullopt;
    }
    return InboundRelayPacket{.packet = std::move(*packet)};
}

} // namespace tgverity
