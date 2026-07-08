#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace tgverity {

// Context attached to every send operation — used for AAD binding.
struct SendContext {
    std::string chatId;         // Telegram chat/peer id
    std::string peerAccount;    // Sender's Telegram account id
    std::string peerDevice;     // Sender's device id
    std::string remoteAccount;  // Receiver's Telegram account id (when known)
};

// In-memory per-peer session: monotonic outbound packet ids + inbound replay cache.
class SessionStore {
public:
    // Allocates the next outbound packet id for a peer.
    [[nodiscard]] std::string nextPacketId(const std::string& peerKey);

    // Records an inbound packet id. Returns false if already seen (replay).
    [[nodiscard]] bool recordInbound(const std::string& peerKey, const std::string& packetId);

    [[nodiscard]] bool hasSeen(const std::string& peerKey, const std::string& packetId) const;

    void clear();

    // Disk persistence: save/load session state so counters + replay cache
    // survive restarts.  Simple newline-delimited text format:
    //   <peerKey> <counter> <packetId1> <packetId2> ...
    // Returns false if file cannot be read/written.
    [[nodiscard]] bool save(const std::string& path) const;
    [[nodiscard]] bool load(const std::string& path);

    [[nodiscard]] std::vector<SendContext> getSendContext(const std::string& chatId) const;

private:
    struct Session {
        std::uint64_t counter = 0;
        std::set<std::string> seen;
        // Track send contexts for AAD binding.
        std::vector<SendContext> contexts;
    };
    std::unordered_map<std::string, Session> _sessions;
};

} // namespace tgverity