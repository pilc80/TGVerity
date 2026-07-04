#include "tgverity/session_store.h"

namespace tgverity {

std::string SessionStore::nextPacketId(const std::string& peerKey) {
    auto& session = _sessions[peerKey];
    return std::to_string(++session.counter);
}

bool SessionStore::recordInbound(const std::string& peerKey, const std::string& packetId) {
    auto& session = _sessions[peerKey];
    return session.seen.insert(packetId).second;
}

bool SessionStore::hasSeen(const std::string& peerKey, const std::string& packetId) const {
    const auto it = _sessions.find(peerKey);
    if (it == _sessions.end()) return false;
    return it->second.seen.count(packetId) > 0;
}

void SessionStore::clear() {
    _sessions.clear();
}

} // namespace tgverity
