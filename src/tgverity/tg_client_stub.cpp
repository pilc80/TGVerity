#include "tgverity/tg_client.h"

#include "tgverity/log.h"

#include <iostream>

namespace tgverity {

std::string StubTelegramClient::sendPacketText(const std::string& chatId,
                                               const std::string& correlationId,
                                               const std::string& packetText) {
    (void)correlationId;
    Logger::instance().log(LogLevel::Info, "stub", "send chat=" + chatId);
    std::cout << "[stub telegram] chat=" << chatId << '\n' << packetText << '\n';
    return "stub";
}

void StubTelegramClient::deleteMessagesRevoke(const std::string& chatId,
                                              const std::vector<std::string>& serverIds) {
    Logger::instance().log(LogLevel::Info, "stub",
                           "delete-revoke chat=" + chatId + " count=" + std::to_string(serverIds.size()));
}

} // namespace tgverity
