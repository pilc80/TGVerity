// Standalone test for tgverity_bridge_shim.
//
// Build (no external deps, links only the two shim TUs):
//   g++ -std=c++17 desktop/tgverity_bridge_shim.cpp \
//                  desktop/tgverity_bridge_shim_tests.cpp \
//                  -o /tmp/tgv_shim_test && /tmp/tgv_shim_test
//
// Verifies: body round-trip; isTgVerityPacket predicate; built wire contains
// no "http"/'@'/'#'; ACK carries ref_id; empty body round-trip; shimSelfTest.

#include "tgverity_bridge_shim.h"

#include <cstdio>
#include <string>

static int g_failures = 0;

#define CHECK(cond) do {                                       \
    if (!(cond)) {                                             \
        std::fprintf(stderr, "FAIL line %d: %s\n",             \
                     __LINE__, #cond);                         \
        ++g_failures;                                          \
    }                                                          \
} while (0)

int main() {
    using namespace tgv_shim;

    // Body with NUL, high-bit, and UTF-8-ish bytes — exercises byte fidelity.
    static const unsigned char kRaw[] = {
        'H','e','l','l','o',',',' ','T','G','V','e','r','i','t','y','!',' ',
        0xf0,0x9f,0x94,0x91,' ','b','i','n',':',' ',
        0x00,0x01,0x02,0xff,' ','e','n','d'
    };
    const std::string body(reinterpret_cast<const char*>(kRaw), sizeof(kRaw));

    // 1. Round-trip body.
    const std::string wire = buildRelayText(body, 7);
    CHECK(isTgVerityPacket(wire));
    const auto p = parse(wire);
    CHECK(p.has_value());
    CHECK(p->type == PacketType::RelayText);
    CHECK(p->packetId == "7");
    CHECK(p->refId == "-");
    CHECK(p->body == body);

    // 2. Predicate: built yes, plain no.
    CHECK(isTgVerityPacket("just a normal message") == false);
    CHECK(parse("just a normal message").has_value() == false);

    // 3. Wire contains no http, @, #.
    CHECK(wire.find("http") == std::string::npos);
    CHECK(wire.find('@') == std::string::npos);
    CHECK(wire.find('#') == std::string::npos);

    // 4. Envelope is fully base32-encoded: no plaintext key leaks in wire.
    CHECK(wire.find("version=") == std::string::npos);
    CHECK(wire.find("body_safe=") == std::string::npos);
    CHECK(wire.find("hello") == std::string::npos);

    // 5. ACK carries ref_id.
    const std::string ack = buildRelayAck(7, 8);
    CHECK(isTgVerityPacket(ack));
    const auto ap = parse(ack);
    CHECK(ap.has_value());
    CHECK(ap->type == PacketType::RelayAck);
    CHECK(ap->refId == "7");
    CHECK(ap->packetId == "8");

    // 6. Empty body round-trips.
    const std::string empty;
    const std::string wireEmpty = buildRelayText(empty, 9);
    const auto pe = parse(wireEmpty);
    CHECK(pe.has_value());
    CHECK(pe->body == empty);

    // 7. Self test (covers predicate + ACK + binary body internally).
    CHECK(shimSelfTest());

    if (g_failures == 0) {
        std::printf("PASS: all tgverity_bridge_shim tests\n");
        return 0;
    }
    std::printf("FAIL: %d assertion(s) failed\n", g_failures);
    return 1;
}
