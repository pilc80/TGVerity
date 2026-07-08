#include "tgverity/session_store.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    using namespace tgverity;

    SessionStore store;
    assert(store.nextPacketId("chat1") == "1");
    assert(store.nextPacketId("chat1") == "2");
    assert(store.nextPacketId("chat2") == "1"); // independent per-peer counter

    assert(store.recordInbound("chat1", "abc"));
    assert(store.hasSeen("chat1", "abc"));
    assert(!store.recordInbound("chat1", "abc")); // replay rejected
    assert(store.recordInbound("chat1", "def"));
    assert(!store.hasSeen("chat1", "xyz"));

    store.clear();
    assert(!store.hasSeen("chat1", "abc"));

    // C1: Disk persistence round-trip.
    const std::string path = "tgverity_session_store.tmp";
    std::remove(path.c_str());

    assert(store.nextPacketId("chatA") == "1");
    assert(store.nextPacketId("chatA") == "2");
    assert(store.recordInbound("chatA", "pkt-100"));
    assert(store.recordInbound("chatA", "pkt-200"));

    assert(store.save(path));
    {
        std::ifstream f(path);
        assert(f.is_open());
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        assert(content.find("chatA") != std::string::npos);
        assert(content.find("2") != std::string::npos);
        assert(content.find("pkt-100") != std::string::npos);
        assert(content.find("pkt-200") != std::string::npos);
    }

    SessionStore loaded;
    assert(loaded.load(path));
    assert(loaded.nextPacketId("chatA") == "3"); // counter preserved
    assert(loaded.hasSeen("chatA", "pkt-100")); // replay cache preserved
    assert(loaded.hasSeen("chatA", "pkt-200"));

    std::remove(path.c_str());

    // C1: Load non-existent file returns true (starts empty).
    SessionStore emptyStore;
    assert(emptyStore.load("/nonexistent/path/that/does/not/exist"));
    assert(emptyStore.nextPacketId("x") == "1");

    std::cout << "session_store_tests passed\n";
    return 0;
}
