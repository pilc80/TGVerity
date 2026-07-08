#include "tgverity/bridge.h"
#include "tgverity/crypto_provider.h"
#include "tgverity/fake_telegram_client.h"
#include "tgverity/relay_packet.h"
#include "tgverity/state_machine.h"

#include <cassert>
#include <fstream>
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

    // -- First message: lifecycle + H3 plaintext wipe --
    // FakeTelegramClient delivers synchronously, so the full lifecycle
    // (MSG sent → tg_sent → ACK → cleanup_done) completes before send() returns.
    const auto pid = aBridge.send("chat1", "hello v0.2");

    // H3: plaintext is wiped on cleanup_done (never stored in memory after lifecycle).
    const auto* ob = aBridge.outbound(pid);
    assert(ob != nullptr);
    assert(ob->status == MessageStatus::cleanup_done);
    assert(ob->plaintext.empty());

    // Bob received, decrypted, suppressed raw Telegram UI paths, rendered virtual message, and cleaned.
    assert(bBridge.inboundCount() == 1);
    const auto* im = bBridge.inbound(pid);
    assert(im != nullptr);
    assert(im->status == MessageStatus::cleanup_done);
    assert(im->plaintext.empty());
    assert(bob.suppressedRenderIds().size() == 1);
    assert(bob.suppressedNotificationIds().size() == 1);
    assert(bob.suppressedEditIds().size() == 1);
    assert(bob.virtualMessages("chat1").size() == 1);
    assert(bob.virtualMessages("chat1").front().packetId == pid);
    // bob.renderVirtualMessage stores plaintext for display (debug output) before cleanup.
    assert(bob.virtualMessages("chat1").front().plaintext == "hello v0.2");

    // Unsafe packet-send options are rejected by the fake adapter contract test double.
    assert(bob.sendPacketText("chat1", "unsafe", "TGVerity", PacketSendOptions{false, true}).empty());

    // Both sides invoked revoke cleanup.
    assert(!alice.deletedIds("chat1").empty());
    assert(!bob.deletedIds("chat1").empty());

    // Replay: re-deliver the same MSG text; Bob ignores it.
    const auto replayWire = RelayPacket::createText(crypto.seal("hello v0.2", "chat1"), pid).toTelegramText();
    bob.receiveFromWire("chat1", "srv-replay", replayWire);
    assert(bBridge.inboundCount() == 1);

    // -- Second message: new packet id, also wiped on cleanup_done --
    const auto pid2 = aBridge.send("chat1", "second");
    assert(pid2 != pid);

    assert(bBridge.inboundCount() == 2);

    // After cleanup_done: wiped.
    const auto* ob2 = aBridge.outbound(pid2);
    assert(ob2 != nullptr);
    assert(ob2->plaintext.empty());
    const auto* im2 = bBridge.inbound(pid2);
    assert(im2 != nullptr);
    assert(im2->plaintext.empty());

    // C1: Session save/load survives through the bridge API.
    const std::string sessPath = "bridge_session.tmp";
    std::remove(sessPath.c_str());
    assert(aBridge.saveSessions(sessPath));
    {
        std::ifstream f(sessPath);
        assert(f.is_open());
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        // chat1 session was used: counters + seen packet ids should be persisted.
        assert(content.find("chat1") != std::string::npos);
    }
    Bridge bBridge2(crypto, bob);
    assert(bBridge2.loadSessions(sessPath));
    assert(bBridge2.inboundCount() == 0); // new bridge starts empty
    std::remove(sessPath.c_str());

    std::cout << "bridge_tests passed\n";
    return 0;
}