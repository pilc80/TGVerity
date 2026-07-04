#include "tgverity/bridge.h"
#include "tgverity/crypto_provider.h"
#include "tgverity/fake_telegram_client.h"
#include "tgverity/relay_packet.h"
#include "tgverity/state_machine.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    using namespace tgverity;

    IdentityCryptoProvider crypto;
    FakeTelegramClient alice;
    FakeTelegramClient bob;
    alice.setPeerDeliverer([&](const std::string& c, const std::string& s, const std::string& t) {
        bob.receiveFromWire(c, s, t);
    });
    bob.setPeerDeliverer([&](const std::string& c, const std::string& s, const std::string& t) {
        alice.receiveFromWire(c, s, t);
    });
    Bridge aBridge(crypto, alice);
    Bridge bBridge(crypto, bob);

    const auto pid = aBridge.send("chat1", "hello v0.2");

    // Alice's outbound message reached cleanup_done end-to-end.
    const auto* ob = aBridge.outbound(pid);
    assert(ob != nullptr);
    assert(ob->plaintext == "hello v0.2");
    assert(ob->status == MessageStatus::cleanup_done);

    // Bob received, decrypted, and cleaned.
    assert(bBridge.inboundCount() == 1);
    const auto* im = bBridge.inbound(pid);
    assert(im != nullptr);
    assert(im->plaintext == "hello v0.2");
    assert(im->status == MessageStatus::cleanup_done);

    // Both sides invoked revoke cleanup.
    assert(!alice.deletedIds("chat1").empty());
    assert(!bob.deletedIds("chat1").empty());

    // Replay: re-deliver the same MSG text; Bob ignores it.
    const auto replayWire = RelayPacket::createText(crypto.seal("hello v0.2", "chat1"), pid).toTelegramText();
    bob.receiveFromWire("chat1", "srv-replay", replayWire);
    assert(bBridge.inboundCount() == 1);

    // A second distinct message gets a new packet id and is received.
    const auto pid2 = aBridge.send("chat1", "second");
    assert(pid2 != pid);
    assert(bBridge.inboundCount() == 2);
    assert(bBridge.inbound(pid2) != nullptr);
    assert(bBridge.inbound(pid2)->plaintext == "second");

    std::cout << "bridge_tests passed\n";
    return 0;
}
