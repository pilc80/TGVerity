#include "tg/td_client.h"

#include "tgverity/relay_packet.h"

#ifdef TGVERITY_USE_TDLIB
#include "td/telegram/td_json_client.h"
#endif

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace tgverity {
namespace {

std::string jsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}

std::string jsonUnescape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            out.push_back(value[i]);
            continue;
        }
        const auto escaped = value[++i];
        switch (escaped) {
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        case '\\': out.push_back('\\'); break;
        case '"': out.push_back('"'); break;
        default: out.push_back(escaped); break;
        }
    }
    return out;
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

std::string textMessageJson(std::int64_t chatId, const std::string& text) {
    std::ostringstream out;
    out << R"({"@type":"sendMessage","chat_id":)" << chatId
        << R"(,"input_message_content":{"@type":"inputMessageText","disable_web_page_preview":true,"text":{"@type":"formattedText","text":")"
        << jsonEscape(text) << R"(","entities":[]}},"@extra":"send"})";
    return out.str();
}

} // namespace

TdClient::TdClient() {
#ifdef TGVERITY_USE_TDLIB
    td_json_client_execute(nullptr, R"({"@type":"setLogVerbosityLevel","new_verbosity_level":0})");
    _client = td_json_client_create();
#endif
}

TdClient::~TdClient() {
#ifdef TGVERITY_USE_TDLIB
    if (_client) {
        td_json_client_send(_client, R"({"@type":"close","@extra":"close"})");
        td_json_client_destroy(_client);
    }
#endif
}

std::string TdClient::version() {
#ifdef TGVERITY_USE_TDLIB
    td_json_client_execute(nullptr, R"({"@type":"setLogVerbosityLevel","new_verbosity_level":0})");
    auto* client = td_json_client_create();
    td_json_client_send(client, R"({"@type":"getOption","name":"version","@extra":"version"})");
    for (int i = 0; i != 20; ++i) {
        if (const auto result = td_json_client_receive(client, 0.5)) {
            const auto text = std::string(result);
            if (contains(text, R"("@extra":"version")")
                || (contains(text, R"("@type":"updateOption")") && contains(text, R"("name":"version")"))) {
                td_json_client_send(client, R"({"@type":"close","@extra":"close"})");
                td_json_client_destroy(client);
                return text;
            }
        }
    }
    td_json_client_send(client, R"({"@type":"close","@extra":"close"})");
    td_json_client_destroy(client);
    return R"({"@type":"error","message":"TDLib version request timed out"})";
#else
    return R"({"@type":"optionValueString","value":"stub"})";
#endif
}

void TdClient::send(const std::string& json) {
#ifdef TGVERITY_USE_TDLIB
    td_json_client_send(_client, json.c_str());
#else
    std::cout << "tdlib stub send: " << json << "\n";
#endif
}

std::optional<std::string> TdClient::receive(double timeoutSeconds) {
#ifdef TGVERITY_USE_TDLIB
    if (const auto result = td_json_client_receive(_client, timeoutSeconds)) {
        return std::string(result);
    }
#endif
    return std::nullopt;
}

int TdClient::login(const AppConfig& config) {
    return ensureReady(config, true);
}

int TdClient::ensureReady(const AppConfig& config, bool allowInteractiveAuth) {
    if (!config.hasTelegramCredentials()) {
        std::cerr << "Set TGVERITY_API_ID and TGVERITY_API_HASH. Optional: TGVERITY_PHONE.\n";
        return 2;
    }
    send(R"({"@type":"getOption","name":"version","@extra":"boot"})");
    const auto baseDir = jsonEscape(config.tdlibDir);
    const auto params = std::string(R"({"@type":"setTdlibParameters","use_test_dc":false,"database_directory":")") + baseDir +
        R"(/db","files_directory":")" + baseDir +
        R"(/files","database_encryption_key":"","use_file_database":true,"use_chat_info_database":true,"use_message_database":true,"use_secret_chats":false,"api_id":)" +
        config.apiId + R"(,"api_hash":")" + jsonEscape(config.apiHash) +
        R"(","system_language_code":"en","device_model":"TGVerity CLI","system_version":"macOS","application_version":"0.2","@extra":"set-params"})";

    std::cout << "Waiting for TDLib authorization state...\n";
    for (;;) {
        auto event = receive(1.0);
        if (!event) continue;
        std::cout << *event << "\n";
        if (contains(*event, "authorizationStateWaitTdlibParameters")) {
            send(params);
        } else if (contains(*event, "authorizationStateWaitEncryptionKey")) {
            send(R"({"@type":"checkDatabaseEncryptionKey","encryption_key":"","@extra":"db-key"})");
        } else if (contains(*event, "authorizationStateWaitPhoneNumber")) {
            if (!allowInteractiveAuth) {
                std::cerr << "TDLib authorization is not complete. Run login first.\n";
                return 2;
            }
            auto phone = config.phone;
            if (phone.empty()) {
                std::cout << "Phone: ";
                if (!std::getline(std::cin, phone) || phone.empty()) {
                    std::cerr << "Phone is required.\n";
                    return 2;
                }
            }
            send(std::string(R"({"@type":"setAuthenticationPhoneNumber","phone_number":")") + jsonEscape(phone) + R"(","@extra":"phone"})");
        } else if (contains(*event, "authorizationStateWaitCode")) {
            if (!allowInteractiveAuth) {
                std::cerr << "TDLib authorization code is required. Run login first.\n";
                return 2;
            }
            std::string code;
            std::cout << "Code: ";
            if (!std::getline(std::cin, code) || code.empty()) {
                std::cerr << "Authentication code is required. Run login interactively.\n";
                return 2;
            }
            send(std::string(R"({"@type":"checkAuthenticationCode","code":")") + jsonEscape(code) + R"(","@extra":"code"})");
        } else if (contains(*event, "authorizationStateWaitPassword")) {
            if (!allowInteractiveAuth) {
                std::cerr << "TDLib 2FA password is required. Run login first.\n";
                return 2;
            }
            std::string password;
            std::cout << "2FA password: ";
            if (!std::getline(std::cin, password) || password.empty()) {
                std::cerr << "2FA password is required. Run login interactively.\n";
                return 2;
            }
            send(std::string(R"({"@type":"checkAuthenticationPassword","password":")") + jsonEscape(password) + R"(","@extra":"password"})");
        } else if (contains(*event, "authorizationStateReady")) {
            std::cout << "TDLib authorization ready\n";
            return 0;
        } else if (contains(*event, R"("@type":"error")")) {
            std::cerr << "TDLib error during authorization\n";
            return 1;
        }
    }
}

int TdClient::chats(const AppConfig& config) {
    if (const auto ready = ensureReady(config, false); ready != 0) return ready;
    send(R"({"@type":"getChats","chat_list":{"@type":"chatListMain"},"limit":50,"@extra":"get-chats"})");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
        auto event = receive(1.0);
        if (!event) continue;
        std::cout << *event << "\n";
        if (contains(*event, R"("@extra":"get-chats")")) {
            return 0;
        }
    }
    return 1;
}

int TdClient::resolve(const AppConfig& config, const std::string& username) {
    if (const auto ready = ensureReady(config, false); ready != 0) return ready;
    auto normalized = username;
    if (!normalized.empty() && normalized.front() == '@') {
        normalized.erase(0, 1);
    }
    send(std::string(R"({"@type":"searchPublicChat","username":")") + jsonEscape(normalized) + R"(","@extra":"resolve"})");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
        auto event = receive(1.0);
        if (!event) continue;
        std::cout << *event << "\n";
        if (contains(*event, R"("@extra":"resolve")")) {
            return contains(*event, R"("@type":"error")") ? 1 : 0;
        }
    }
    return 1;
}

int TdClient::sendText(const AppConfig& config, std::int64_t chatId, const std::string& text) {
    if (const auto ready = ensureReady(config, false); ready != 0) return ready;
    send(textMessageJson(chatId, text));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (std::chrono::steady_clock::now() < deadline) {
        auto event = receive(1.0);
        if (!event) continue;
        std::cout << *event << "\n";
        if (contains(*event, R"("@type":"updateMessageSendSucceeded")")) return 0;
        if (contains(*event, R"("@type":"updateMessageSendFailed")") || contains(*event, R"("@type":"error")")) return 1;
    }
    std::cerr << "Timed out waiting for Telegram send confirmation\n";
    return 1;
}

int TdClient::watch(const AppConfig& config, std::optional<std::int64_t> chatId) {
    if (const auto ready = ensureReady(config, false); ready != 0) return ready;
    std::cout << "Watching TDLib updates. Ctrl+C to stop.\n";
    while (true) {
        auto event = receive(1.0);
        if (!event) continue;
        if (chatId && !contains(*event, std::string(R"("chat_id":)") + std::to_string(*chatId))) {
            continue;
        }
        std::cout << *event << "\n";
        const auto textKey = std::string(R"("text":")");
        const auto pos = event->find(textKey);
        if (pos != std::string::npos) {
            const auto start = pos + textKey.size();
            const auto end = event->find('"', start);
            if (end != std::string::npos) {
                const auto text = jsonUnescape(event->substr(start, end - start));
                if (auto packet = RelayPacket::fromTelegramText(text)) {
                    std::cout << "TGVerity packet: type=" << packet->type
                              << " packet_id=" << packet->packetId
                              << " body=" << packet->body << "\n";
                }
            }
        }
    }
}

} // namespace tgverity
