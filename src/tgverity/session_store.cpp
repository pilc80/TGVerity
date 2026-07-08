#include "tgverity/session_store.h"

#include "tgverity/log.h"

#include <fstream>
#include <sstream>

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

bool SessionStore::save(const std::string& path) const {
    std::ofstream f(path, std::ios::out | std::ios::trunc);
    if (!f.is_open()) {
        Logger::instance().log(LogLevel::Warn, "session_store", "save failed: cannot open " + path);
        return false;
    }
    for (const auto& [key, sess] : _sessions) {
        f << key << "\t" << sess.counter << "\t";
        for (const auto& pkt : sess.seen) {
            f << pkt << "|";
        }
        f << "\n";
    }
    f.flush();
    return f.good();
}

bool SessionStore::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        // No file yet — ok, starts empty.
        return true;
    }
    _sessions.clear();
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string key, token;
        if (!std::getline(iss, key, '\t')) continue;
        if (!std::getline(iss, token, '\t')) continue;
        auto& session = _sessions[key];
        try {
            session.counter = std::stoull(token);
        } catch (...) {
            Logger::instance().log(LogLevel::Warn, "session_store",
                                   "bad counter for peer " + key);
            continue;
        }
        // Remaining tokens (pipe-delimited) are seen packet ids.
        std::string seenToken;
        while (std::getline(iss, seenToken, '|')) {
            if (!seenToken.empty()) {
                session.seen.insert(seenToken);
            }
        }
    }
    Logger::instance().log(LogLevel::Info, "session_store",
                           "loaded " + std::to_string(_sessions.size()) + " peers from " + path);
    return true;
}

std::vector<SendContext> SessionStore::getSendContext(const std::string& chatId) const {
    const auto it = _sessions.find(chatId);
    if (it == _sessions.end()) return {};
    return it->second.contexts;
}

} // namespace tgverity