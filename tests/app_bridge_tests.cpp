#include "app/config.h"
#include "tgverity/p2p_socket.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

int main() {
    using namespace tgverity;

    const auto missing = AppConfig::fromEnvironment();
    assert(!missing.hasTelegramCredentials());

    setenv("TGVERITY_API_ID", "12345", 1);
    setenv("TGVERITY_API_HASH", "abc", 1);
    setenv("TGVERITY_PHONE", "+10000000000", 1);
    const auto config = AppConfig::fromEnvironment();
    assert(config.apiId == "12345");
    assert(config.apiHash == "abc");
    assert(config.phone == "+10000000000");
    assert(config.hasTelegramCredentials());

    const auto endpoint = parseEndpoint("127.0.0.1:7777");
    assert(endpoint.has_value());
    assert(endpoint->host == "127.0.0.1");
    assert(endpoint->port == 7777);
    assert(!parseEndpoint("bad").has_value());

    std::cout << "app_bridge_tests passed\n";
    return 0;
}
