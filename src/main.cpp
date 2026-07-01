#include "app/config.h"
#include "tg/td_client.h"
#include "tgverity/bridge.h"
#include "tgverity/p2p_frame.h"
#include "tgverity/p2p_socket.h"
#include "tgverity/relay_packet.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

namespace {

void usage() {
    std::cout << "TGVerity v0.1 macOS harness\n"
              << "Commands:\n"
              << "  tdlib-version                  Print TDLib version / stub marker\n"
              << "  login                          Start TDLib login flow\n"
              << "  chats                          List recent chat IDs JSON\n"
              << "  send-normal <chat> <text>      Send normal Telegram text\n"
              << "  send-relay <chat> <text>       Send visible TGVerity relay packet\n"
              << "  watch [chat]                   Print TDLib updates and parse relay packets\n"
              << "  relay-pack <text>              Build visible Telegram text relay packet\n"
              << "  relay-parse <packet>           Parse visible Telegram text relay packet\n"
              << "  p2p-frame <text>               Build prototype P2P frame\n"
              << "  p2p-listen <port>              Accept one framed P2P message\n"
              << "  p2p-connect <host:port> <text> Send one framed P2P message\n";
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
        tgverity::TdClient client;
        return client.login(tgverity::AppConfig::fromEnvironment());
    }

    if (command == "chats") {
        tgverity::TdClient client;
        return client.chats();
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
            text = tgverity::Bridge().buildRelayTelegramText(text).text;
        }
        tgverity::TdClient client;
        return client.sendText(*chatId, text);
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
        tgverity::TdClient client;
        return client.watch(chatId);
    }

    if (command == "relay-pack") {
        if (argc < 3) {
            std::cerr << "relay-pack requires text\n";
            return 2;
        }
        std::cout << tgverity::RelayPacket::createText(argv[2]).toTelegramText() << "\n";
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
        std::cout << "type=" << parsed->type << " body=" << parsed->body << "\n";
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

    usage();
    return 2;
}
