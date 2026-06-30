#include "tgverity/tg_client.h"

#include <iostream>

namespace tgverity {

void StubTelegramClient::sendRelayPacketText(const std::string& chatId, const std::string& packetText) {
    std::cout << "[stub telegram] chat=" << chatId << '\n' << packetText << '\n';
}

} // namespace tgverity
