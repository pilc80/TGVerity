// TGVerity C ABI implementation
//
// Forward-declares extern "C" functions in tgverity_core.h that delegate
// to the core RelayPacket, buildRelayText, buildRelayAck, and related
// free functions.  Eliminates code duplication with the desktop shim.

#include "tgverity/tgverity_core.h"

#include "tgverity/relay_packet.h"

#include <cstring>

size_t tgv_build_relay_text(const char* body, size_t body_len,
                            uint64_t packet_id,
                            char* out, size_t out_cap) {
    const tgverity::RelayPacket pkt = tgverity::RelayPacket::createText(
        tgverity::Bytes(body, body_len), std::to_string(packet_id));
    const std::string wire = pkt.toTelegramText();
    if (wire.size() > out_cap) return 0;
    std::memcpy(out, wire.data(), wire.size());
    return static_cast<size_t>(wire.size());
}

size_t tgv_build_relay_ack(uint64_t ref_id, uint64_t ack_packet_id,
                           char* out, size_t out_cap) {
    const tgverity::RelayPacket pkt = tgverity::RelayPacket::createAck(
        std::to_string(ref_id), std::to_string(ack_packet_id));
    const std::string wire = pkt.toTelegramText();
    if (wire.size() > out_cap) return 0;
    std::memcpy(out, wire.data(), wire.size());
    return static_cast<size_t>(wire.size());
}

int tgv_is_packet(const char* text, size_t text_len) {
    return tgverity::isRelayPacketText(std::string(text, text_len)) ? 1 : 0;
}

int tgv_parse(const char* text, size_t text_len, tgv_parsed_t* out) {
    const auto parsed = tgverity::RelayPacket::fromTelegramText(
        std::string(text, text_len));
    if (!parsed) return 0;

    out->type = (parsed->type == "relay_ack") ? 1 : 0;
    std::strncpy(out->packet_id, parsed->packetId.c_str(), sizeof(out->packet_id) - 1);
    out->packet_id[sizeof(out->packet_id) - 1] = '\0';
    std::strncpy(out->ref_id, parsed->refId.c_str(), sizeof(out->ref_id) - 1);
    out->ref_id[sizeof(out->ref_id) - 1] = '\0';

    if (parsed->body.size() > sizeof(out->body)) {
        return 0; // body too large
    }
    std::memcpy(out->body, parsed->body.data(), parsed->body.size());
    out->body_len = parsed->body.size();

    return 1;
}