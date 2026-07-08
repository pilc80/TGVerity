#include "tgverity/relay_packet.h"

#include <array>
#include <cstdint>
#include <sstream>
#include <string_view>

namespace tgverity {
namespace {

constexpr std::string_view kPrefix = "TGVerity secure packet. Open with TGVerity to read.\nv1:";
constexpr std::string_view kProtocol = "tgverity";
constexpr std::string_view kVersion = "1";
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
    std::ostringstream out;
    out << "protocol=" << kProtocol
        << "\nversion=" << kVersion
        << "\ncrypto_suite=" << packet.cryptoSuite
        << "\ntype=" << packet.type
        << "\npacket_id=" << packet.packetId
        << "\nref_id=" << packet.refId
        << "\nbody_safe=" << encodeSafe(packet.body);
    return out.str();
}

std::optional<RelayPacket> parseEnvelope(const std::string& envelope) {
    RelayPacket packet;
    bool haveProtocol = false, haveVersion = false, haveType = false, haveId = false, haveSuite = false;
    std::istringstream stream(envelope);
    std::string line;
    while (std::getline(stream, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        const auto key = line.substr(0, pos);
        const auto value = line.substr(pos + 1);
        if (key == "protocol") {
            if (value == kProtocol) haveProtocol = true;
        } else if (key == "version") {
            if (value == kVersion) haveVersion = true;
        } else if (key == "crypto_suite") {
            try {
                packet.cryptoSuite = static_cast<std::uint16_t>(std::stoul(value));
                haveSuite = true;
            } catch (...) {
                return std::nullopt;
            }
        } else if (key == "type") {
            packet.type = value;
            haveType = true;
        } else if (key == "packet_id") {
            packet.packetId = value;
            haveId = true;
        } else if (key == "ref_id") {
            packet.refId = value;
        } else if (key == "body_safe") {
            auto decoded = decodeSafe(value);
            if (!decoded) return std::nullopt;
            packet.body = *decoded;
        }
    }
    if (!(haveProtocol && haveVersion && haveSuite && haveType && haveId)) return std::nullopt;
    if (packet.type != "relay_text" && packet.type != "relay_ack") return std::nullopt;
    if (packet.type == "relay_ack" && (packet.refId == "-" || packet.refId.empty())) return std::nullopt;
    // L4: relay_text must have a non-empty body (empty body is valid only for ACK).
    // Matches shim (desktop/tgverity_bridge_shim.cpp:166) to prevent drift.
    if (packet.type == "relay_text" && packet.body.empty()) return std::nullopt;
    return packet;
}

} // namespace

RelayPacket RelayPacket::createText(Bytes body, std::string packetId) {
    RelayPacket p;
    p.cryptoSuite = static_cast<std::uint16_t>(CryptoSuite::Identity);
    p.type = "relay_text";
    p.packetId = std::move(packetId);
    p.refId = "-";
    p.body = std::move(body);
    return p;
}

RelayPacket RelayPacket::createAck(std::string refId, std::string packetId) {
    RelayPacket p;
    p.cryptoSuite = static_cast<std::uint16_t>(CryptoSuite::Identity);
    p.type = "relay_ack";
    p.packetId = std::move(packetId);
    p.refId = std::move(refId);
    p.body = {};
    return p;
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
