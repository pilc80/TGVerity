#pragma once

#include <string>
#include <vector>

namespace tgverity {

// Adapter -> core callbacks. The correlation id is the TGVerity packet id
// (mirrors TDLib's @extra correlation), so the bridge needs no send-id map.
class AdapterListener {
public:
    virtual ~AdapterListener() = default;
    virtual void onMessageIdBound(const std::string& chatId,
                                  const std::string& correlationId,
                                  const std::string& serverId) = 0;
    virtual void onIncomingMessage(const std::string& chatId,
                                   const std::string& serverId,
                                   const std::string& text) = 0;
};

// Platform adapter contract (TDLib / tdesktop fork / MTProto lib all implement this).
class TelegramAdapter {
public:
    virtual ~TelegramAdapter() = default;

    // Sends opaque packet text. correlationId is echoed back via onMessageIdBound.
    // Returns the adapter's local send id (informational; may be ignored).
    virtual std::string sendPacketText(const std::string& chatId,
                                       const std::string& correlationId,
                                       const std::string& packetText) = 0;

    // Best-effort delete-for-all (revoke). Single call, <=100 ids, never self-then-revoke.
    virtual void deleteMessagesRevoke(const std::string& chatId,
                                      const std::vector<std::string>& serverIds) = 0;

    virtual void setListener(AdapterListener* listener) = 0;
};

class StubTelegramClient final : public TelegramAdapter {
public:
    std::string sendPacketText(const std::string& chatId, const std::string& correlationId,
                               const std::string& packetText) override;
    void deleteMessagesRevoke(const std::string& chatId, const std::vector<std::string>& serverIds) override;
    void setListener(AdapterListener* listener) override { _listener = listener; }
private:
    AdapterListener* _listener = nullptr;
};

// Placeholder until real TDLib wiring lands. Throws on use.
class TdlibTelegramClient final : public TelegramAdapter {
public:
    std::string sendPacketText(const std::string& chatId, const std::string& correlationId,
                               const std::string& packetText) override;
    void deleteMessagesRevoke(const std::string& chatId, const std::vector<std::string>& serverIds) override;
    void setListener(AdapterListener* listener) override { _listener = listener; }
private:
    AdapterListener* _listener = nullptr;
};

} // namespace tgverity
