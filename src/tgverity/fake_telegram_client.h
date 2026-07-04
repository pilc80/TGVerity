#pragma once

#include "tgverity/tg_client.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tgverity {

// In-memory adapter for tests + CLI selftest. Two fakes wired together simulate
// the Telegram wire: a send on one delivers to the peer's onIncomingMessage.
class FakeTelegramClient final : public TelegramAdapter {
public:
    std::string sendPacketText(const std::string& chatId,
                               const std::string& correlationId,
                               const std::string& packetText) override;
    void deleteMessagesRevoke(const std::string& chatId,
                              const std::vector<std::string>& serverIds) override;
    void setListener(AdapterListener* listener) override { _listener = listener; }

    using Deliverer = std::function<void(const std::string& chatId,
                                         const std::string& serverId,
                                         const std::string& text)>;
    void setPeerDeliverer(Deliverer deliverer) { _deliver = std::move(deliverer); }

    // Simulates an inbound message off the wire (fires onIncomingMessage).
    void receiveFromWire(const std::string& chatId,
                         const std::string& serverId,
                         const std::string& text);

    [[nodiscard]] std::size_t sentCount() const { return _sent; }
    [[nodiscard]] const std::vector<std::string>& deletedIds(const std::string& chatId) const;

private:
    AdapterListener* _listener = nullptr;
    Deliverer _deliver;
    std::uint64_t _counter = 0;
    std::size_t _sent = 0;
    std::unordered_map<std::string, std::vector<std::string>> _deleted;
};

} // namespace tgverity
