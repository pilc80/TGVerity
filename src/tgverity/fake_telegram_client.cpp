#include "tgverity/fake_telegram_client.h"

#include "tgverity/log.h"

namespace tgverity {

std::string FakeTelegramClient::sendPacketText(const std::string& chatId,
                                               const std::string& correlationId,
                                               const std::string& packetText) {
    ++_counter;
    const std::string sendId = "local-" + std::to_string(_counter);
    const std::string serverId = "srv-" + std::to_string(_counter);
    ++_sent;
    Logger::instance().log(LogLevel::Debug, "fake",
                           "send chat=" + chatId + " corr=" + correlationId
                           + " sendId=" + sendId + " serverId=" + serverId);
    // Server accepts + assigns id before peer delivery (so the sender has its
    // server id by the time the peer's ACK comes back over the synchronous wire).
    if (_listener) _listener->onMessageIdBound(chatId, correlationId, serverId);
    if (_deliver) _deliver(chatId, serverId, packetText);
    return sendId;
}

void FakeTelegramClient::deleteMessagesRevoke(const std::string& chatId,
                                              const std::vector<std::string>& serverIds) {
    auto& bucket = _deleted[chatId];
    for (const auto& id : serverIds) {
        bucket.push_back(id);
    }
    Logger::instance().log(LogLevel::Info, "fake",
                           "delete-revoke chat=" + chatId + " count=" + std::to_string(serverIds.size()));
}

void FakeTelegramClient::receiveFromWire(const std::string& chatId,
                                         const std::string& serverId,
                                         const std::string& text) {
    Logger::instance().log(LogLevel::Debug, "fake", "recv chat=" + chatId + " serverId=" + serverId);
    if (_listener) _listener->onIncomingMessage(chatId, serverId, text);
}

const std::vector<std::string>& FakeTelegramClient::deletedIds(const std::string& chatId) const {
    static const std::vector<std::string> empty;
    const auto it = _deleted.find(chatId);
    return it == _deleted.end() ? empty : it->second;
}

} // namespace tgverity
