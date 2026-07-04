#pragma once

#include "tgverity/crypto_provider.h"
#include "tgverity/session_store.h"
#include "tgverity/state_machine.h"
#include "tgverity/tg_client.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace tgverity {

struct OutboundMessage {
    std::string packetId;
    std::string plaintext;
    MessageStatus status = MessageStatus::local_pending;
    std::string serverId;
    std::string ackServerId;
};

struct InboundMessage {
    std::string packetId;
    std::string plaintext;
    MessageStatus status = MessageStatus::cleanup_pending;
    std::string msgServerId;
    std::string ackServerId;
};

// Orchestrates the TGVerity lifecycle over a platform adapter. Implements
// AdapterListener to receive id-binding and inbound-message events.
class Bridge final : public AdapterListener {
public:
    Bridge(CryptoProvider& crypto, TelegramAdapter& adapter);

    // Sends a TGVerity message; returns the allocated packet id.
    std::string send(const std::string& chatId, const std::string& plaintext);

    void onMessageIdBound(const std::string& chatId,
                          const std::string& correlationId,
                          const std::string& serverId) override;
    void onIncomingMessage(const std::string& chatId,
                           const std::string& serverId,
                           const std::string& text) override;

    [[nodiscard]] const OutboundMessage* outbound(const std::string& packetId) const;
    [[nodiscard]] const InboundMessage* inbound(const std::string& packetId) const;
    [[nodiscard]] std::size_t inboundCount() const { return _inbound.size(); }
    [[nodiscard]] std::size_t outboundCount() const { return _outbound.size(); }

private:
    CryptoProvider& _crypto;
    TelegramAdapter& _adapter;
    SessionStore _sessions;
    std::unordered_map<std::string, OutboundMessage> _outbound;     // packetId -> msg
    std::unordered_map<std::string, InboundMessage> _inbound;       // packetId -> msg
    std::unordered_map<std::string, std::string> _ackForInbound;    // ack packetId -> inbound packetId
};

} // namespace tgverity
