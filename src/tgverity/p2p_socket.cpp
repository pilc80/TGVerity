#include "tgverity/p2p_socket.h"

#include "tgverity/p2p_frame.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <limits>

namespace tgverity {

std::optional<Endpoint> parseEndpoint(const std::string& value) {
    const auto pos = value.rfind(':');
    if (pos == std::string::npos || pos == 0 || pos + 1 >= value.size()) {
        return std::nullopt;
    }
    const auto host = value.substr(0, pos);
    const auto portText = value.substr(pos + 1);
    char* end = nullptr;
    const auto port = std::strtol(portText.c_str(), &end, 10);
    if (!end || *end != '\0' || port <= 0 || port > std::numeric_limits<std::uint16_t>::max()) {
        return std::nullopt;
    }
    return Endpoint{.host = host, .port = static_cast<std::uint16_t>(port)};
}

int runP2PListen(std::uint16_t port) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || ::listen(fd, 1) < 0) {
        std::cerr << "listen failed: " << std::strerror(errno) << "\n";
        ::close(fd);
        return 1;
    }
    std::cout << "listening on " << port << "\n";
    const int client = ::accept(fd, nullptr, nullptr);
    if (client < 0) {
        std::cerr << "accept failed: " << std::strerror(errno) << "\n";
        ::close(fd);
        return 1;
    }
    std::string buffer(4096, '\0');
    const auto n = ::read(client, buffer.data(), buffer.size());
    if (n > 0) {
        buffer.resize(static_cast<std::size_t>(n));
        auto decoded = decodeFrame(buffer);
        if (decoded) {
            std::cout << "received: " << *decoded << "\n";
        } else {
            std::cout << "received malformed frame\n";
        }
    }
    ::close(client);
    ::close(fd);
    return 0;
}

int runP2PConnect(const Endpoint& endpoint, const std::string& text) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    const auto port = std::to_string(endpoint.port);
    if (::getaddrinfo(endpoint.host.c_str(), port.c_str(), &hints, &result) != 0 || !result) {
        std::cerr << "resolve failed\n";
        return 1;
    }
    const int fd = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (fd < 0 || ::connect(fd, result->ai_addr, result->ai_addrlen) < 0) {
        std::cerr << "connect failed: " << std::strerror(errno) << "\n";
        if (fd >= 0) ::close(fd);
        ::freeaddrinfo(result);
        return 1;
    }
    const auto frame = encodeFrame(text);
    ::write(fd, frame.data(), frame.size());
    std::cout << "sent: " << text << "\n";
    ::close(fd);
    ::freeaddrinfo(result);
    return 0;
}

} // namespace tgverity
