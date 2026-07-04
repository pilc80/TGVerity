#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>

namespace tgverity {

// In-memory per-peer session: monotonic outbound packet ids + inbound replay cache.
class SessionStore {
public:
    // Allocates the next outbound packet id for a peer.
    [[nodiscard]] std::string nextPacketId(const std::string& peerKey);

    // Records an inbound packet id. Returns false if already seen (replay).
    [[nodiscard]] bool recordInbound(const std::string& peerKey, const std::string& packetId);

    [[nodiscard]] bool hasSeen(const std::string& peerKey, const std::string& packetId) const;

    void clear();

private:
    struct Session {
        std::uint64_t counter = 0;
        std::set<std::string> seen;
    };
    std::unordered_map<std::string, Session> _sessions;
};

} // namespace tgverity
