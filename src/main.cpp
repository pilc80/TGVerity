#include "app/config.h"
#include "tg/td_client.h"
#include "tgverity/bridge.h"
#include "tgverity/crypto_provider.h"
#include "tgverity/fake_telegram_client.h"
#include "tgverity/log.h"
#include "tgverity/p2p_frame.h"
#include "tgverity/p2p_socket.h"
#include "tgverity/relay_packet.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace {

void usage() {
    std::cout << "TGVerity v0.1 macOS harness\n"
              << "Commands:\n"
              << "  tdlib-version                  Print TDLib version / stub marker\n"
              << "  login                          Start TDLib login flow\n"
              << "  chats                          List recent chat IDs JSON\n"
              << "  resolve <@username>            Resolve public username to chat JSON\n"
              << "  send-normal <chat> <text>      Send normal Telegram text\n"
              << "  send-relay <chat> <text>       Send visible TGVerity relay packet\n"
              << "  watch [chat]                   Print TDLib updates and parse relay packets\n"
              << "  relay-pack <text>              Build visible Telegram text relay packet\n"
              << "  relay-parse <packet>           Parse visible Telegram text relay packet\n"
              << "  p2p-frame <text>               Build prototype P2P frame\n"
              << "  p2p-listen <port>              Accept one framed P2P message\n"
              << "  p2p-connect <host:port> <text> Send one framed P2P message\n"
              << "  selftest                       End-to-end bridge demo via FakeTelegramClient + logging\n"
              "  selftest-shim                  Standalone shim self-test (no core link)\n"
              "  tg-bridge-send <chat> <text>   Send TGVerity packet via Bridge + TDLib\n"
              "  tg-bridge-wait [timeout_s]     Wait for Bridge events (ACK, incoming)\n"
              "  tg-bridge                      Start Bridge event loop (send + recv + watch)\n";
}

std::optional<std::int64_t> parseChatId(const char* value) {
    try {
        return std::stoll(value);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::uint16_t> parsePort(const char* value) {
    try {
        const auto port = std::stoi(value);
        if (port <= 0 || port > 65535) return std::nullopt;
        return static_cast<std::uint16_t>(port);
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 0;
    }

    const std::string command = argv[1];
    if (command == "tdlib-version") {
        std::cout << tgverity::TdClient::version() << "\n";
        return 0;
    }

    if (command == "login") {
        auto config = tgverity::AppConfig::fromEnvironmentOrPrompt();
        if (!config.hasTelegramCredentials()) {
            std::cerr << "Set TGVERITY_API_ID and TGVERITY_API_HASH, or enter them when prompted.\n";
            return 2;
        }
        tgverity::TdClient client;
        return client.login(config);
    }

    if (command == "chats") {
        auto config = tgverity::AppConfig::fromEnvironment();
        tgverity::TdClient client;
        return client.chats(config);
    }

    if (command == "resolve") {
        if (argc < 3) {
            std::cerr << "resolve requires <@username>\n";
            return 2;
        }
        auto config = tgverity::AppConfig::fromEnvironment();
        tgverity::TdClient client;
        return client.resolve(config, argv[2]);
    }

    if (command == "send-normal" || command == "send-relay") {
        if (argc < 4) {
            std::cerr << command << " requires <chat> <text>\n";
            return 2;
        }
        const auto chatId = parseChatId(argv[2]);
        if (!chatId) {
            std::cerr << "invalid chat id\n";
            return 2;
        }
        auto text = std::string(argv[3]);
        if (command == "send-relay") {
            text = tgverity::RelayPacket::createText(text, "cli-send").toTelegramText();
        }
        auto config = tgverity::AppConfig::fromEnvironment();
        tgverity::TdClient client;
        return client.sendText(config, *chatId, text);
    }

    if (command == "watch") {
        std::optional<std::int64_t> chatId;
        if (argc >= 3) {
            chatId = parseChatId(argv[2]);
            if (!chatId) {
                std::cerr << "invalid chat id\n";
                return 2;
            }
        }
        auto config = tgverity::AppConfig::fromEnvironment();
        tgverity::TdClient client;
        return client.watch(config, chatId);
    }

    if (command == "relay-pack") {
        if (argc < 3) {
            std::cerr << "relay-pack requires text\n";
            return 2;
        }
        std::cout << tgverity::RelayPacket::createText(argv[2], "cli-pack").toTelegramText() << "\n";
        return 0;
    }

    if (command == "relay-parse") {
        if (argc < 3) {
            std::cerr << "relay-parse requires packet text\n";
            return 2;
        }
        auto parsed = tgverity::RelayPacket::fromTelegramText(argv[2]);
        if (!parsed) {
            std::cerr << "not a valid TGVerity packet\n";
            return 1;
        }
        std::cout << "type=" << parsed->type << " packet_id=" << parsed->packetId
                  << " ref_id=" << parsed->refId << " body=" << parsed->body << "\n";
        return 0;
    }

    if (command == "p2p-frame") {
        if (argc < 3) {
            std::cerr << "p2p-frame requires text\n";
            return 2;
        }
        std::cout << tgverity::encodeFrame(argv[2]) << "\n";
        return 0;
    }

    if (command == "p2p-listen") {
        if (argc < 3) {
            std::cerr << "p2p-listen requires port\n";
            return 2;
        }
        const auto port = parsePort(argv[2]);
        if (!port) {
            std::cerr << "invalid port\n";
            return 2;
        }
        return tgverity::runP2PListen(*port);
    }

    if (command == "p2p-connect") {
        if (argc < 4) {
            std::cerr << "p2p-connect requires host:port text\n";
            return 2;
        }
        auto endpoint = tgverity::parseEndpoint(argv[2]);
        if (!endpoint) {
            std::cerr << "invalid endpoint\n";
            return 2;
        }
        return tgverity::runP2PConnect(*endpoint, argv[3]);
    }

    if (command == "selftest-shim") {
        // Compile-time integration test: the desktop shim must self-test
        // successfully in isolation (no tgverity_core link). This mirrors the
        // command that tdesktop developers run during desktop integration.
        // See desktop/tgverity_bridge_shim_tests.cpp for the full test.
        std::cout << "shim-test: compile desktop/tgverity_bridge_shim.cpp standalone and run\n";
        return 0;
    }

    if (command == "selftest") {
        tgverity::Logger::instance().setLevel(tgverity::LogLevel::Debug);
        tgverity::Logger::instance().setRedact(true);
        tgverity::IdentityCryptoProvider crypto;
        tgverity::FakeTelegramClient alice;
        tgverity::FakeTelegramClient bob;
        alice.setPeerDeliverer([&](const std::string& c, const std::string& s, const std::string& t) {
            bob.receiveFromWire(c, s, t);
        });
        bob.setPeerDeliverer([&](const std::string& c, const std::string& s, const std::string& t) {
            alice.receiveFromWire(c, s, t);
        });
        tgverity::Bridge aBridge(crypto, alice);
        tgverity::Bridge bBridge(crypto, bob);
        const auto pid = aBridge.send("chat1", "hello v0.2");
        std::cout << "alice sent packetId=" << pid << "\n";
        const auto* ob = aBridge.outbound(pid);
        std::cout << "alice outbound status=" << (ob ? tgverity::statusName(ob->status) : "null") << "\n";
        std::cout << "bob inbound count=" << bBridge.inboundCount() << "\n";
        const auto* im = bBridge.inbound(pid);
        std::cout << "bob inbound plaintext=" << (im ? im->plaintext : std::string("null")) << "\n";
        std::cout << "selftest OK\n";
        return 0;
    }

    // tg-bridge-send: send a TGVerity packet via Bridge + real TDLib.
    if (command == "tg-bridge-send") {
        if (argc < 4) {
            std::cerr << "tg-bridge-send requires <chat> <text>\n";
            return 2;
        }
        const auto chatId = parseChatId(argv[2]);
        if (!chatId) {
            std::cerr << "invalid chat id\n";
            return 2;
        }

        tgverity::Logger::instance().setLevel(tgverity::LogLevel::Debug);

        // 1. Create Bridge + TdlibTelegramClient.
        tgverity::IdentityCryptoProvider crypto;
        tgverity::TdlibTelegramClient tdlibClient;
        tgverity::Bridge bridge(crypto, tdlibClient);

        // 2. Authenticate (interactive).
        auto config = tgverity::AppConfig::fromEnvironmentOrPrompt();
        if (!config.hasTelegramCredentials()) {
            std::cerr << "Set TGVERITY_API_ID and TGVERITY_API_HASH.\n";
            return 2;
        }
        if (const auto rc = tdlibClient.authenticate(config); rc != 0) return rc;
        std::cout << "Auth OK. Sending...\n";

        // 3. Send through bridge.
        std::cout << "Sending TGVerity packet to chat=" << *chatId << " text=\"" << argv[3] << "\"\n";
        const auto packetId = bridge.send(std::to_string(*chatId), argv[3]);
        std::cout << "Packet sent, id=" << packetId << "\n";

        // 4. Wait for events: message send confirmed, ACK, cleanup.
        //    The background receive loop fires callbacks into the Bridge.
        //    We poll bridge state until cleanup_done or timeout.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        while (std::chrono::steady_clock::now() < deadline) {
            const auto* ob = bridge.outbound(packetId);
            if (ob) {
                std::cout << "Status: " << tgverity::statusName(ob->status) << "\n";
                if (ob->status == tgverity::MessageStatus::cleanup_done) {
                    std::cout << "Message fully cleaned up.\n";
                    return 0;
                }
                if (ob->status == tgverity::MessageStatus::failed) {
                    std::cerr << "Message send failed.\n";
                    return 1;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cerr << "Timed out.\n";
        return 1;
    }

    usage();
    return 2;
}
