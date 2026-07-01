#include "tgverity/relay_packet.h"

#include <array>
#include <cstdint>
#include <sstream>
#include <string_view>

namespace tgverity {
namespace {

constexpr std::string_view kPrefix = "TGVerity secure packet. Open with TGVerity to read.\nv1:";
constexpr std::array<char, 32> kAlphabet = {
    'A','B','C','D','E','F','G','H','J','K','M','N','P','Q','R','S',
    'T','V','W','X','Y','Z','2','3','4','5','6','7','8','9','0','1'
};

std::string encodeSafe(const std::string& input) {
    std::string out;
    uint32_t buffer = 0;
    int bits = 0;
    for (unsigned char c : input) {
        buffer = (buffer << 8) | c;
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            out.push_back(kAlphabet[(buffer >> bits) & 31]);
        }
    }
    if (bits > 0) {
        out.push_back(kAlphabet[(buffer << (5 - bits)) & 31]);
    }
    return out;
}

int decodeValue(char c) {
    for (int i = 0; i < static_cast<int>(kAlphabet.size()); ++i) {
        if (kAlphabet[i] == c) return i;
    }
    return -1;
}

std::optional<std::string> decodeSafe(const std::string& input) {
    uint32_t buffer = 0;
    int bits = 0;
    std::string out;
    for (char c : input) {
        int value = decodeValue(c);
        if (value < 0) return std::nullopt;
        buffer = (buffer << 5) | static_cast<uint32_t>(value);
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buffer >> bits) & 0xff));
        }
    }
    return out;
}

std::string envelopeFor(const RelayPacket& packet) {
    return "protocol=tgverity\nversion=" + packet.version + "\ntype=" + packet.type +
           "\npacket_id=" + packet.packetId + "\nbody_safe=" + encodeSafe(packet.body);
}

std::optional<RelayPacket> parseEnvelope(const std::string& envelope) {
    RelayPacket packet;
    std::istringstream stream(envelope);
    std::string line;
    while (std::getline(stream, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        auto key = line.substr(0, pos);
        auto value = line.substr(pos + 1);
        if (key == "version") packet.version = value;
        else if (key == "type") packet.type = value;
        else if (key == "packet_id") packet.packetId = value;
        else if (key == "body_safe") {
            auto decodedBody = decodeSafe(value);
            if (!decodedBody) return std::nullopt;
            packet.body = *decodedBody;
        }
    }
    if (packet.version != "1" || packet.type.empty() || packet.packetId.empty()) {
        return std::nullopt;
    }
    return packet;
}

} // namespace

RelayPacket RelayPacket::createText(std::string body) {
    return RelayPacket{"1", "relay_text", "prototype-1", std::move(body)};
}

std::string RelayPacket::toTelegramText() const {
    return std::string(kPrefix) + encodeSafe(envelopeFor(*this));
}

std::optional<RelayPacket> RelayPacket::fromTelegramText(const std::string& text) {
    if (!isRelayPacketText(text)) return std::nullopt;
    auto encoded = text.substr(kPrefix.size());
    auto decoded = decodeSafe(encoded);
    if (!decoded) return std::nullopt;
    return parseEnvelope(*decoded);
}

bool isRelayPacketText(const std::string& text) {
    return text.rfind(kPrefix, 0) == 0;
}

} // namespace tgverity
