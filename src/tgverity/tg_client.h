#pragma once

#include "app/config.h"

#include <memory>
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
struct PacketSendOptions {
    bool disableLinkPreview = true;
    bool allowEntities = false;
};

struct VirtualMessage {
    std::string packetId;
    std::string plaintext;
};

class TelegramAdapter {
public:
    virtual ~TelegramAdapter() = default;

    // Sends opaque packet text. correlationId is echoed back via onMessageIdBound.
    // Implementations must send as plain text, no entities/formatting, and disable
    // link previews where the platform supports it.
    // Returns the adapter's local send id (informational; may be ignored).
    virtual std::string sendPacketText(const std::string& chatId,
                                       const std::string& correlationId,
                                       const std::string& packetText,
                                       PacketSendOptions options = {}) = 0;

    // Best-effort delete-for-all (revoke). Single call, <=100 ids, never self-then-revoke.
    virtual void deleteMessagesRevoke(const std::string& chatId,
                                      const std::vector<std::string>& serverIds) = 0;

    // Raw Telegram packet hygiene hooks. Platform implementations may no-op only
    // in non-UI harnesses; GUI adapters must suppress raw packet UI/notification/edit paths.
    virtual void suppressRawRender(const std::string& serverId) = 0;
    virtual void suppressRawNotification(const std::string& serverId) = 0;
    virtual void suppressRawEdit(const std::string& serverId) = 0;
    virtual void renderVirtualMessage(const std::string& chatId, const VirtualMessage& message) = 0;

    virtual void setListener(AdapterListener* listener) = 0;
};

class StubTelegramClient final : public TelegramAdapter {
public:
    std::string sendPacketText(const std::string& chatId, const std::string& correlationId,
                               const std::string& packetText,
                               PacketSendOptions options = {}) override;
    void deleteMessagesRevoke(const std::string& chatId, const std::vector<std::string>& serverIds) override;
    void suppressRawRender(const std::string& serverId) override;
    void suppressRawNotification(const std::string& serverId) override;
    void suppressRawEdit(const std::string& serverId) override;
    void renderVirtualMessage(const std::string& chatId, const VirtualMessage& message) override;
    void setListener(AdapterListener* listener) override { _listener = listener; }
private:
    AdapterListener* _listener = nullptr;
};

// TDLib-backed adapter with authentication (only when built with TGVERITY_USE_TDLIB).
#ifdef TGVERITY_USE_TDLIB
class TdlibTelegramClient final : public TelegramAdapter {
public:
    struct Impl;
    TdlibTelegramClient();
    ~TdlibTelegramClient();
    TdlibTelegramClient(const TdlibTelegramClient&) = delete;
    TdlibTelegramClient& operator=(const TdlibTelegramClient&) = delete;

    int authenticate();
    int authenticate(const AppConfig& config);

    std::string sendPacketText(const std::string& chatId, const std::string& correlationId,
                               const std::string& packetText,
                               PacketSendOptions options = {}) override;
    void deleteMessagesRevoke(const std::string& chatId, const std::vector<std::string>& serverIds) override;
    void suppressRawRender(const std::string& serverId) override;
    void suppressRawNotification(const std::string& serverId) override;
    void suppressRawEdit(const std::string& serverId) override;
    void renderVirtualMessage(const std::string& chatId, const VirtualMessage& message) override;
    void setListener(AdapterListener* listener) override;
private:
    std::unique_ptr<Impl> _impl;
};
#else
// Stub when TDLib is not available — satisfies the linker but does nothing.
class TdlibTelegramClient final : public TelegramAdapter {
public:
    TdlibTelegramClient() = default;
    ~TdlibTelegramClient() = default;
    int authenticate() { return -1; }
    int authenticate(const AppConfig&) { return -1; }
    std::string sendPacketText(const std::string&, const std::string&, const std::string&,
                               PacketSendOptions = {}) override { return {}; }
    void deleteMessagesRevoke(const std::string&, const std::vector<std::string>&) override {}
    void suppressRawRender(const std::string&) override {}
    void suppressRawNotification(const std::string&) override {}
    void suppressRawEdit(const std::string&) override {}
    void renderVirtualMessage(const std::string&, const VirtualMessage&) override {}
    void setListener(AdapterListener*) override {}
};
#endif

} // namespace tgverity
