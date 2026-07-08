// TGVerity C ABI bridge
//
// Stable C interface exposed by tgverity-core so the tdesktop shim can
// call the core packet codec without duplicating wire-format code.
//
// Usage in the shim:
//   #include "tgverity_core.h"  // when linking against core
//   Then replace shim's own base32/envelope codec with these calls.

#include <cstdint>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Build a relay_text wire-text packet.
//   body     = provider output bytes.
//   packetId = monotonic session counter.
//   out      = output buffer (must be at least out_cap bytes).
//   Returns number of bytes written, or 0 if body is too large.
size_t tgv_build_relay_text(const char* body, size_t body_len,
                            uint64_t packet_id,
                            char* out, size_t out_cap);

// Build a relay_ack wire-text packet.
//   refId         = packet_id of the message being acknowledged.
//   ackPacketId   = the ACK packet's own session counter.
size_t tgv_build_relay_ack(uint64_t ref_id, uint64_t ack_packet_id,
                           char* out, size_t out_cap);

// True iff text begins with the TGVerity v1 prefix.
int tgv_is_packet(const char* text, size_t text_len);

// Struct for parsed packet output.
typedef struct {
    // 0 = relay_text, 1 = relay_ack
    int type;
    char packet_id[64];
    char ref_id[64];
    // body bytes (may contain NUL; body_len tells the real size)
    char body[4096];
    size_t body_len;
} tgv_parsed_t;

// Parse a wire-text packet. Returns 1 on success, 0 if not TGVerity or malformed.
int tgv_parse(const char* text, size_t text_len, tgv_parsed_t* out);

#ifdef __cplusplus
}
#endif