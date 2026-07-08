#include "tgverity/tg_client.h"

#include "tgverity/log.h"

#include <stdexcept>

namespace tgverity {

std::string TdlibTelegramClient::sendPacketText(const std::string& chatId,
                                                const std::string& correlationId,
                                                const std::string& packetText,
                                                PacketSendOptions options) {
    (void)chatId;
    (void)correlationId;
    (void)packetText;
    (void)options;
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

void TdlibTelegramClient::suppressRawRender(const std::string& serverId) {
    (void)serverId;
    Logger::instance().log(LogLevel::Warn, "tdlib", "suppressRawRender not implemented (shim)");
}

void TdlibTelegramClient::suppressRawNotification(const std::string& serverId) {
    (void)serverId;
    Logger::instance().log(LogLevel::Warn, "tdlib", "suppressRawNotification not implemented (shim)");
}

void TdlibTelegramClient::suppressRawEdit(const std::string& serverId) {
    (void)serverId;
    Logger::instance().log(LogLevel::Warn, "tdlib", "suppressRawEdit not implemented (shim)");
}

void TdlibTelegramClient::renderVirtualMessage(const std::string& chatId, const VirtualMessage& message) {
    (void)chatId;
    (void)message;
    Logger::instance().log(LogLevel::Warn, "tdlib", "renderVirtualMessage not implemented (shim)");
}

} // namespace tgverity
