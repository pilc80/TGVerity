#include "tgverity/relay_packet.h"
#include "tgverity/p2p_frame.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace tgverity;

    auto packet = RelayPacket::createText("hello relay");
    auto wire = packet.toTelegramText();
    assert(isRelayPacketText(wire));
    assert(wire.find("TGVerity secure packet. Open with TGVerity to read.") == 0);
    assert(wire.find("http") == std::string::npos);
    assert(wire.find('@') == std::string::npos);
    assert(wire.find('#') == std::string::npos);

    auto parsed = RelayPacket::fromTelegramText(wire);
    assert(parsed.has_value());
    assert(parsed->type == "relay_text");
    assert(parsed->body == "hello relay");

    auto multiline = RelayPacket::createText("line1\nline2\ntype=evil").toTelegramText();
    auto parsedMultiline = RelayPacket::fromTelegramText(multiline);
    assert(parsedMultiline.has_value());
    assert(parsedMultiline->type == "relay_text");
    assert(parsedMultiline->body == "line1\nline2\ntype=evil");

    assert(!isRelayPacketText("normal telegram message"));
    assert(!RelayPacket::fromTelegramText("TGVerity secure packet. Open with TGVerity to read.\nv1:not-valid").has_value());

    auto framed = encodeFrame("p2p hello");
    auto decoded = decodeFrame(framed);
    assert(decoded.has_value());
    assert(*decoded == "p2p hello");
    assert(!decodeFrame("bad").has_value());
    assert(!decodeFrame("TGV1 0000000g\n").has_value());
    assert(!decodeFrame("TGV1 00000001x\na").has_value());

    std::cout << "relay_packet_tests passed\n";
    return 0;
}
