#include "app/config.h"
#include "tgverity/bridge.h"
#include "tgverity/p2p_socket.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

int main() {
    using namespace tgverity;

    auto missing = AppConfig::fromEnvironment();
    assert(!missing.hasTelegramCredentials());

    setenv("TGVERITY_API_ID", "12345", 1);
    setenv("TGVERITY_API_HASH", "abc", 1);
    setenv("TGVERITY_PHONE", "+10000000000", 1);
    auto config = AppConfig::fromEnvironment();
    assert(config.apiId == "12345");
    assert(config.apiHash == "abc");
    assert(config.phone == "+10000000000");
    assert(config.hasTelegramCredentials());

    Bridge bridge;
    auto out = bridge.buildRelayTelegramText("hello");
    assert(isRelayPacketText(out.text));
    auto in = bridge.tryParseTelegramText(out.text);
    assert(in.has_value());
    assert(in->packet.body == "hello");
    assert(!bridge.tryParseTelegramText("normal").has_value());

    auto endpoint = parseEndpoint("127.0.0.1:7777");
    assert(endpoint.has_value());
    assert(endpoint->host == "127.0.0.1");
    assert(endpoint->port == 7777);
    assert(!parseEndpoint("bad").has_value());

    std::cout << "app_bridge_tests passed\n";
    return 0;
}
