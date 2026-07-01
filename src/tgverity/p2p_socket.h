#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace tgverity {

struct Endpoint {
    std::string host;
    std::uint16_t port = 0;
};

[[nodiscard]] std::optional<Endpoint> parseEndpoint(const std::string& value);
int runP2PListen(std::uint16_t port);
int runP2PConnect(const Endpoint& endpoint, const std::string& text);

} // namespace tgverity
