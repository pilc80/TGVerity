// Compile-time drift test: shim and core must produce identical wire output.
//
// This file is NOT part of the main tgverity_core library (no public headers).
// It is compiled as a standalone binary by CMakeLists.txt and runs as a test.
// If it fails, tgverity-core and the desktop shim have diverged.

#include "tgverity/relay_packet.h"
#include "tgverity/tgverity_core.h"

#include <cassert>
#include <cstdio>
#include <string>

static int g_failures = 0;

#define CHECK(cond, msg) do {                                     \
    if (!(cond)) {                                                \
        std::fprintf(stderr, "DRIFT %s\n", msg);                  \
        ++g_failures;                                             \
    }                                                             \
} while (0)

int main() {
    using namespace tgverity;

    // 1. buildRelayText: C ABI vs core
    {
        const std::string body = "test body with 0xff\xff bytes \x00\x01";
        const size_t cap = 8192;
        char out[cap];

        // C ABI path
        const size_t abi_len = tgv_build_relay_text(body.data(), body.size(),
                                                     42, out, cap);
        std::string abi_wire(out, abi_len);

        // Core path
        const RelayPacket pkt = RelayPacket::createText(
            Bytes(body), "42");
        std::string core_wire = pkt.toTelegramText();

        CHECK(abi_len == core_wire.size(), "abi_len != core_len");
        CHECK(std::memcmp(out, core_wire.data(), core_wire.size()) == 0,
              "abi body != core body");
    }

    // 2. buildRelayAck: C ABI vs core
    {
        const size_t cap = 8192;
        char out[cap];

        const size_t abi_len = tgv_build_relay_ack(7, 8, out, cap);
        std::string abi_wire(out, abi_len);

        const RelayPacket pkt = RelayPacket::createAck("7", "8");
        std::string core_wire = pkt.toTelegramText();

        CHECK(abi_len == core_wire.size(), "ack abi_len != core_len");
        CHECK(std::memcmp(out, core_wire.data(), core_wire.size()) == 0,
              "ack abi body != core body");
    }

    // 3. isPacket: C ABI vs core
    {
        CHECK(tgv_is_packet("hello", 5) == 0, "isPacket 'hello' should be false");
        CHECK(tgv_is_packet("TGVerity secure packet. Open with TGVerity to read.\n"
                            "v1:AAAA", 64) == 1,
              "isPacket valid prefix should be true");
    }

    // 4. parse: C ABI vs core
    {
        const RelayPacket pkt = RelayPacket::createText("hello", "1");
        std::string wire = pkt.toTelegramText();

        tgv_parsed_t parsed;
        int rc = tgv_parse(wire.data(), wire.size(), &parsed);
        CHECK(rc == 1, "parse should succeed");
        CHECK(parsed.type == 0, "parse type should be relay_text");
        CHECK(std::strcmp(parsed.packet_id, "1") == 0, "parse packet_id");
        CHECK(std::strcmp(parsed.ref_id, "-") == 0, "parse ref_id");
        CHECK(parsed.body_len == 5, "parse body_len");
        CHECK(std::memcmp(parsed.body, "hello", 5) == 0, "parse body");
    }

    if (g_failures == 0) {
        std::printf("PASS: tgverity_core shim drift test\n");
        return 0;
    }
    std::printf("FAIL: %d drift test(s)\n", g_failures);
    return 1;
}