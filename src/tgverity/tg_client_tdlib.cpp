#include "tgverity/tg_client.h"

#include "tgverity/log.h"

#include <stdexcept>

namespace tgverity {

std::string TdlibTelegramClient::sendPacketText(const std::string& chatId,
                                                const std::string& correlationId,
                                                const std::string& packetText) {
    (void)chatId;
    (void)correlationId;
    (void)packetText;
    Logger::instance().log(LogLevel::Warn, "tdlib", "sendPacketText not implemented (shim)");
    throw std::runtime_error("TdlibTelegramClient::sendPacketText not implemented");
}

void TdlibTelegramClient::deleteMessagesRevoke(const std::string& chatId,
                                               const std::vector<std::string>& serverIds) {
    (void)chatId;
    (void)serverIds;
    Logger::instance().log(LogLevel::Warn, "tdlib", "deleteMessagesRevoke not implemented (shim)");
    throw std::runtime_error("TdlibTelegramClient::deleteMessagesRevoke not implemented");
}

} // namespace tgverity
