// TGVerity Desktop Bridge Shim (v0.2)
//
// Standalone C++17, zero external deps. Mirrors the tgverity-core v1 packet
// format BYTE-FOR-BYTE so a packet built by tgverity-core round-trips through
// this shim (and vice versa).
//
// Format (authoritative: docs/v0.2-design.md, src/tgverity/relay_packet.cpp):
//
//   wire_text = kPrefix + base32(envelope)
//
//   envelope (key=value, "\n" separated, this exact key order, no trailing nl):
//     protocol=tgverity
//     version=1
//     crypto_suite=0
//     type=relay_text | relay_ack
//     packet_id=<counter>
//     ref_id=<id|"-">            (ACK carries original packet_id)
//     body_safe=base32(provider_output_bytes)
//
//   kPrefix = "TGVerity secure packet. Open with TGVerity to read.\nv1:"
//   base32 alphabet (32 chars, no I/L/O/U):
//     A B C D E F G H J K M N P Q R S T V W X Y Z 2 3 4 5 6 7 8 9 0 1
//   base32 codec = same bit-packing as relay_packet.cpp encodeSafe/decodeSafe.
//
// DISCLAIMER: the base32 codec + envelope logic is DUPLICATED from
// tgverity-core to keep this shim dependency-free for in-tree tdesktop builds.
// Plan: replace the duplication with a thin C ABI into tgverity_core
// (see desktop/README.md "C ABI plan").

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace tgv_shim {

enum class PacketType : int {
    RelayText = 0,
    RelayAck  = 1,
};

struct ParsedPacket {
    PacketType type = PacketType::RelayText;
    std::string packetId;   // counter as ASCII digits
    std::string refId;      // "-" when absent (relay_text)
    std::string body;       // raw provider-output bytes (plaintext under IdentityCryptoProvider)
};

// Build a relay_text wire-text packet.
//   bodyBytes = provider output (plaintext under IdentityCryptoProvider).
//   packetId  = monotonic session counter.
std::string buildRelayText(const std::string& bodyBytes, std::uint64_t packetId);

// Build a relay_ack wire-text packet.
//   refId       = packet_id of the message being acknowledged.
//   ackPacketId = the ACK packet's own session counter.
std::string buildRelayAck(std::uint64_t refId, std::uint64_t ackPacketId);

// True iff text begins with the TGVerity v1 prefix.
bool isTgVerityPacket(const std::string& text);

// Parse a wire-text packet. Returns nullopt if not TGVerity or malformed.
std::optional<ParsedPacket> parse(const std::string& wireText);

// Self test: round-trip, predicate, ACK ref_id, no http/@/#, empty body.
// Returns true on pass. Intended for `ctest`/smoke wiring; also exercised by
// tgverity_bridge_shim_tests.cpp.
bool shimSelfTest();

} // namespace tgv_shim
