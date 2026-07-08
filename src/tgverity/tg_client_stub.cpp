#include "tgverity/tg_client.h"

#include "tgverity/log.h"

#include <iostream>

namespace tgverity {

std::string StubTelegramClient::sendPacketText(const std::string& chatId,
                                               const std::string& correlationId,
                                               const std::string& packetText,
                                               PacketSendOptions options) {
    (void)correlationId;
    if (!options.disableLinkPreview || options.allowEntities) {
        Logger::instance().log(LogLevel::Warn, "stub", "unsafe packet send options rejected");
        return {};
    }
    Logger::instance().log(LogLevel::Info, "stub", "send chat=" + chatId);
    std::cout << "[stub telegram] chat=" << chatId << '\n' << packetText << '\n';
    return "stub";
}

void StubTelegramClient::deleteMessagesRevoke(const std::string& chatId,
                                              const std::vector<std::string>& serverIds) {
    Logger::instance().log(LogLevel::Info, "stub",
                           "delete-revoke chat=" + chatId + " count=" + std::to_string(serverIds.size()));
}

void StubTelegramClient::suppressRawRender(const std::string& serverId) {
    Logger::instance().log(LogLevel::Info, "stub", "suppress-render serverId=" + serverId);
}

void StubTelegramClient::suppressRawNotification(const std::string& serverId) {
    Logger::instance().log(LogLevel::Info, "stub", "suppress-notification serverId=" + serverId);
}

void StubTelegramClient::suppressRawEdit(const std::string& serverId) {
    Logger::instance().log(LogLevel::Info, "stub", "suppress-edit serverId=" + serverId);
}

void StubTelegramClient::renderVirtualMessage(const std::string& chatId, const VirtualMessage& message) {
    Logger::instance().log(LogLevel::Info, "stub",
                           "render-virtual chat=" + chatId + " packetId=" + message.packetId);
    std::cout << "[stub virtual] chat=" << chatId << " packet=" << message.packetId << '\n';
}

} // namespace tgverity
