#include "tgverity/relay_packet.h"
#include "tgverity/p2p_frame.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using namespace tgverity;

    const auto packet = RelayPacket::createText("hello relay", "pkt-1");
    const auto wire = packet.toTelegramText();
    assert(isRelayPacketText(wire));
    assert(wire.find("TGVerity secure packet. Open with TGVerity to read.") == 0);
    assert(wire.find("http") == std::string::npos);
    assert(wire.find('@') == std::string::npos);
    assert(wire.find('#') == std::string::npos);

    const auto parsed = RelayPacket::fromTelegramText(wire);
    assert(parsed.has_value());
    assert(parsed->type == "relay_text");
    assert(parsed->packetId == "pkt-1");
    assert(parsed->refId == "-");
    assert(parsed->cryptoSuite == static_cast<std::uint16_t>(CryptoSuite::Identity));
    assert(parsed->body == "hello relay");

    const auto multiline = RelayPacket::createText("line1\nline2\ntype=evil", "pkt-2").toTelegramText();
    const auto pm = RelayPacket::fromTelegramText(multiline);
    assert(pm.has_value());
    assert(pm->body == "line1\nline2\ntype=evil");

    const RelayPacket ack = RelayPacket::createAck("pkt-1", "ack-1");
    assert(ack.type == "relay_ack");
    assert(ack.refId == "pkt-1");
    const auto ackWire = ack.toTelegramText();
    const auto ackParsed = RelayPacket::fromTelegramText(ackWire);
    assert(ackParsed.has_value());
    assert(ackParsed->type == "relay_ack");
    assert(ackParsed->refId == "pkt-1");
    assert(ackParsed->packetId == "ack-1");

    // ACK with empty refId is rejected as malformed.
    RelayPacket badAck = RelayPacket::createAck("pkt-1", "ack-x");
    badAck.refId = "";
    assert(!RelayPacket::fromTelegramText(badAck.toTelegramText()).has_value());

    assert(!isRelayPacketText("normal telegram message"));
    assert(!RelayPacket::fromTelegramText(
        "TGVerity secure packet. Open with TGVerity to read.\nv1:not-valid").has_value());

    const auto framed = encodeFrame("p2p hello");
    const auto decoded = decodeFrame(framed);
    assert(decoded.has_value());
    assert(*decoded == "p2p hello");
    assert(!decodeFrame("bad").has_value());
    assert(!decodeFrame("TGV1 0000000g\n").has_value());
    assert(!decodeFrame("TGV1 00000001x\na").has_value());

    std::cout << "relay_packet_tests passed\n";
    return 0;
}
