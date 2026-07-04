// TGVerity Desktop Bridge Shim (v0.2) — implementation.
//
// Mirrors src/tgverity/relay_packet.cpp's codec + envelope logic. Kept
// self-contained (no core headers) so it can compile inside the tdesktop
// tree. See tgverity_bridge_shim.h for the format contract.

#include "tgverity_bridge_shim.h"

#include <array>
#include <cstdint>
#include <sstream>
#include <string_view>

namespace tgv_shim {
namespace {

constexpr std::string_view kPrefix =
    "TGVerity secure packet. Open with TGVerity to read.\nv1:";

// Identical to relay_packet.cpp kAlphabet (no I, L, O, U).
constexpr std::array<char, 32> kAlphabet = {
    'A','B','C','D','E','F','G','H','J','K','M','N','P','Q','R','S',
    'T','V','W','X','Y','Z','2','3','4','5','6','7','8','9','0','1'
};

// O(1) reverse lookup; rejects anything outside the safe alphabet (returns -1),
// matching relay_packet.cpp decodeValue semantics.
std::array<int, 256> buildReverse() {
    std::array<int, 256> t;
    for (int i = 0; i < 256; ++i) t[i] = -1;
    for (int i = 0; i < 32; ++i) {
        t[static_cast<unsigned char>(kAlphabet[i])] = i;
    }
    return t;
}

const std::array<int, 256>& reverseTable() {
    static const auto t = buildReverse();
    return t;
}

// Bit-packed base32 encoder, identical packing to relay_packet.cpp encodeSafe.
std::string encodeSafe(const std::string& input) {
    std::string out;
    std::uint32_t buffer = 0;
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

// Returns false on any character outside the safe alphabet.
bool decodeSafe(const std::string& input, std::string& out) {
    const auto& rev = reverseTable();
    std::uint32_t buffer = 0;
    int bits = 0;
    out.clear();
    for (char c : input) {
        int value = rev[static_cast<unsigned char>(c)];
        if (value < 0) return false;
        buffer = (buffer << 5) | static_cast<std::uint32_t>(value);
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buffer >> bits) & 0xff));
        }
    }
    return true;
}

std::string envelopeFor(PacketType type,
                        std::uint64_t packetId,
                        const std::string& refId,
                        const std::string& bodyBytes) {
    std::string e;
    e.reserve(96 + bodyBytes.size() * 8 / 5 + 1);
    e += "protocol=tgverity\n";
    e += "version=1\n";
    e += "crypto_suite=0\n";
    e += "type=";
    e += (type == PacketType::RelayAck ? "relay_ack" : "relay_text");
    e += "\npacket_id=";
    e += std::to_string(packetId);
    e += "\nref_id=";
    e += refId;
    e += "\nbody_safe=";
    e += encodeSafe(bodyBytes);
    return e;
}

} // namespace

std::string buildRelayText(const std::string& bodyBytes, std::uint64_t packetId) {
    return std::string(kPrefix) +
           encodeSafe(envelopeFor(PacketType::RelayText, packetId, "-", bodyBytes));
}

std::string buildRelayAck(std::uint64_t refId, std::uint64_t ackPacketId) {
    return std::string(kPrefix) +
           encodeSafe(envelopeFor(PacketType::RelayAck, ackPacketId,
                                  std::to_string(refId), std::string()));
}

bool isTgVerityPacket(const std::string& text) {
    return text.rfind(kPrefix, 0) == 0;
}

std::optional<ParsedPacket> parse(const std::string& wireText) {
    if (!isTgVerityPacket(wireText)) return std::nullopt;
    std::string envelope;
    if (!decodeSafe(wireText.substr(kPrefix.size()), envelope)) {
        return std::nullopt;
    }

    ParsedPacket p;
    p.refId = "-";  // default when ref_id line is absent
    bool haveType = false, havePacketId = false, haveBodySafe = false;
    std::string bodySafe;

    std::istringstream stream(envelope);
    std::string line;
    while (std::getline(stream, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        const auto key = line.substr(0, pos);
        const auto value = line.substr(pos + 1);
        if (key == "protocol") {
            if (value != "tgverity") return std::nullopt;
        } else if (key == "version") {
            if (value != "1") return std::nullopt;
        } else if (key == "crypto_suite") {
            // Body is opaque provider output; shim does no crypto. Suite value
            // is parsed but not validated here — core owns crypto policy.
        } else if (key == "type") {
            if (value == "relay_text") {
                p.type = PacketType::RelayText;
                haveType = true;
            } else if (value == "relay_ack") {
                p.type = PacketType::RelayAck;
                haveType = true;
            } else {
                return std::nullopt;
            }
        } else if (key == "packet_id") {
            p.packetId = value;
            havePacketId = true;
        } else if (key == "ref_id") {
            p.refId = value;
        } else if (key == "body_safe") {
            bodySafe = value;
            haveBodySafe = true;
        }
        // Unknown keys ignored for forward-compat.
    }

    if (!haveType || !havePacketId) return std::nullopt;
    if (p.type == PacketType::RelayText && !haveBodySafe) return std::nullopt;

    if (!bodySafe.empty()) {
        std::string raw;
        if (!decodeSafe(bodySafe, raw)) return std::nullopt;
        p.body = std::move(raw);
    }
    return p;
}

bool shimSelfTest() {
    // relay_text round-trip with binary body (incl. NUL + 0xff).
    static const unsigned char kRaw[] = {
        'h','i',' ',1,2,3,0,0xff,'x','y','z'
    };
    const std::string body(reinterpret_cast<const char*>(kRaw), sizeof(kRaw));
    const auto wire = buildRelayText(body, 42);
    if (!isTgVerityPacket(wire)) return false;
    // Wire must stay inside the safe transport set: no http/@/#.
    if (wire.find("http") != std::string::npos) return false;
    if (wire.find('@') != std::string::npos) return false;
    if (wire.find('#') != std::string::npos) return false;
    const auto parsed = parse(wire);
    if (!parsed) return false;
    if (parsed->type != PacketType::RelayText) return false;
    if (parsed->packetId != "42") return false;
    if (parsed->refId != "-") return false;
    if (parsed->body != body) return false;

    // ACK carries ref_id.
    const auto ack = buildRelayAck(42, 43);
    const auto ap = parse(ack);
    if (!ap) return false;
    if (ap->type != PacketType::RelayAck) return false;
    if (ap->refId != "42") return false;
    if (ap->packetId != "43") return false;

    // Plain text is not a TGVerity packet.
    if (isTgVerityPacket("hello world")) return false;
    if (parse("hello world").has_value()) return false;

    // Empty body round-trips.
    const auto wireEmpty = buildRelayText(std::string(), 9);
    const auto pe = parse(wireEmpty);
    if (!pe) return false;
    if (!pe->body.empty()) return false;

    return true;
}

} // namespace tgv_shim
