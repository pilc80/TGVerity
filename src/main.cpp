#include "tgverity/relay_packet.h"
#include "tgverity/p2p_frame.h"

#include <iostream>
#include <string>

namespace {

void usage() {
    std::cout << "TGVerity v0.1 macOS harness\n"
              << "Commands:\n"
              << "  relay-pack <text>      Build visible Telegram text relay packet\n"
              << "  relay-parse <packet>   Parse visible Telegram text relay packet\n"
              << "  p2p-frame <text>       Build prototype P2P frame\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 0;
    }

    const std::string command = argv[1];
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

    usage();
    return 2;
}
