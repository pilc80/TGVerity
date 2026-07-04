#include "tgverity/session_store.h"

#include <cassert>
#include <iostream>

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

    std::cout << "session_store_tests passed\n";
    return 0;
}
