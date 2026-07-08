#include "tgverity/bridge.h"

#include "tgverity/log.h"
#include "tgverity/relay_packet.h"

namespace tgverity {

Bridge::Bridge(CryptoProvider& crypto, TelegramAdapter& adapter)
    : _crypto(crypto), _adapter(adapter) {
    _adapter.setListener(this);
}

std::string Bridge::send(const std::string& chatId, const std::string& plaintext) {
    const auto packetId = _sessions.nextPacketId(chatId);
    const auto sealed = _crypto.seal(plaintext, chatId); // aad binds payload to chat
    const RelayPacket packet = RelayPacket::createText(sealed, packetId);

    OutboundMessage msg;
    msg.packetId = packetId;
    msg.plaintext = plaintext;
    msg.status = MessageStatus::local_pending;
    _outbound[packetId] = msg;

    Logger::instance().log(LogLevel::Info, "bridge",
                           "send chat=" + chatId + " packetId=" + packetId
                           + " plaintext=" + redactSecret(plaintext, Logger::instance().secureRedact()));

    _adapter.sendPacketText(chatId, packetId, packet.toTelegramText());
    return packetId;
}

void Bridge::onMessageIdBound(const std::string& chatId,
                              const std::string& correlationId,
                              const std::string& serverId) {
    // Outbound MSG (sender side)?
    auto oit = _outbound.find(correlationId);
    if (oit != _outbound.end()) {
        if (StateMachine::transition(oit->second.status, MessageStatus::tg_sent)) {
            oit->second.serverId = serverId;
            Logger::instance().log(LogLevel::Debug, "bridge",
                                   "outbound " + correlationId + " -> tg_sent serverId=" + serverId);
        }
        return;
    }
    // ACK we sent (receiver side) -> clean the inbound MSG + this ACK.
    auto ait = _ackForInbound.find(correlationId);
    if (ait != _ackForInbound.end()) {
        const auto inboundId = ait->second;
        auto iit = _inbound.find(inboundId);
        if (iit != _inbound.end()) {
            iit->second.ackServerId = serverId;
            _adapter.deleteMessagesRevoke(chatId, {iit->second.msgServerId, serverId});
            StateMachine::transition(iit->second.status, MessageStatus::cleanup_done);
            iit->second.clearPlaintext();
            Logger::instance().log(LogLevel::Debug, "bridge", "inbound " + inboundId + " cleanup_done");
        }
    }
}

void Bridge::onIncomingMessage(const std::string& chatId,
                               const std::string& serverId,
                               const std::string& text) {
    const auto parsed = RelayPacket::fromTelegramText(text);
    if (!parsed) {
        Logger::instance().log(LogLevel::Debug, "bridge", "inbound non-tgverity, ignore serverId=" + serverId);
        return;
    }

    if (parsed->type == "relay_ack") {
        // Suppress all raw Telegram UI paths for the ACK packet.
        _adapter.suppressRawRender(serverId);
        _adapter.suppressRawNotification(serverId);
        _adapter.suppressRawEdit(serverId);

        auto oit = _outbound.find(parsed->refId);
        if (oit == _outbound.end()) {
            Logger::instance().log(LogLevel::Warn, "bridge", "ACK ref=" + parsed->refId + " unknown");
            return;
        }
        oit->second.ackServerId = serverId;
        StateMachine::transition(oit->second.status, MessageStatus::tgverity_ack);
        StateMachine::transition(oit->second.status, MessageStatus::cleanup_pending);
        // The synchronous adapter binds the server id before delivering to the peer,
        // so serverId is known by the time the ACK returns. Async adapters must
        // retry cleanup once both ids are bound (and add locking) when they land.
        if (!oit->second.serverId.empty()) {
            _adapter.deleteMessagesRevoke(chatId, {oit->second.serverId, serverId});
            StateMachine::transition(oit->second.status, MessageStatus::cleanup_done);
            oit->second.clearPlaintext();
        }
        Logger::instance().log(LogLevel::Info, "bridge",
                               "ACK ref=" + parsed->refId + " state=" + statusName(oit->second.status));
        return;
    }

    // relay_text
    _adapter.suppressRawRender(serverId);
    _adapter.suppressRawNotification(serverId);
    _adapter.suppressRawEdit(serverId);
    if (!_sessions.recordInbound(chatId, parsed->packetId)) {
        Logger::instance().log(LogLevel::Warn, "bridge", "replay packetId=" + parsed->packetId + " ignored");
        return;
    }
    const auto opened = _crypto.open(parsed->body, chatId);
    if (!opened) {
        Logger::instance().log(LogLevel::Error, "bridge", "decrypt failed packetId=" + parsed->packetId);
        // Transition to failed so stuck messages never sit in cleanup_pending.
        InboundMessage im;
        im.packetId = parsed->packetId;
        im.status = MessageStatus::failed;
        im.msgServerId = serverId;
        _inbound[parsed->packetId] = im;
        return;
    }

    InboundMessage im;
    im.packetId = parsed->packetId;
    im.plaintext = *opened;
    im.status = MessageStatus::cleanup_pending;
    im.msgServerId = serverId;
    _inbound[parsed->packetId] = im;
    _adapter.renderVirtualMessage(chatId, VirtualMessage{parsed->packetId, *opened});
    Logger::instance().log(LogLevel::Info, "bridge",
                           "recv chat=" + chatId + " packetId=" + parsed->packetId
                           + " plaintext=" + redactSecret(*opened, Logger::instance().secureRedact()));

    // Emit ACK; register ackForInbound before send so the synchronous
    // onMessageIdBound callback during send can find it.
    const auto ackPacketId = _sessions.nextPacketId(chatId);
    _ackForInbound[ackPacketId] = parsed->packetId;
    const RelayPacket ack = RelayPacket::createAck(parsed->packetId, ackPacketId);
    _adapter.sendPacketText(chatId, ackPacketId, ack.toTelegramText());
}

const OutboundMessage* Bridge::outbound(const std::string& packetId) const {
    const auto it = _outbound.find(packetId);
    return it == _outbound.end() ? nullptr : &it->second;
}

const InboundMessage* Bridge::inbound(const std::string& packetId) const {
    const auto it = _inbound.find(packetId);
    return it == _inbound.end() ? nullptr : &it->second;
}

} // namespace tgverity
