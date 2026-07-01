#include "tg/td_client.h"

#include "tgverity/bridge.h"

#ifdef TGVERITY_USE_TDLIB
#include "td/telegram/td_json_client.h"
#endif

#include <chrono>
#include <iostream>
#include <sstream>
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
    if (!config.hasTelegramCredentials()) {
        std::cerr << "Set TGVERITY_API_ID and TGVERITY_API_HASH. Optional: TGVERITY_PHONE.\n";
        return 2;
    }
    send(R"({"@type":"getOption","name":"version","@extra":"boot"})");
    const auto baseDir = jsonEscape(config.tdlibDir);
    const auto params = std::string(R"({"@type":"setTdlibParameters","use_test_dc":false,"database_directory":")") + baseDir +
        R"(/db","files_directory":")" + baseDir +
        R"(/files","use_file_database":true,"use_chat_info_database":true,"use_message_database":true,"use_secret_chats":false,"api_id":)" +
        config.apiId + R"(,"api_hash":")" + jsonEscape(config.apiHash) +
        R"(","system_language_code":"en","device_model":"TGVerity CLI","system_version":"macOS","application_version":"0.1","@extra":"set-params"})";

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
            auto phone = config.phone;
            if (phone.empty()) {
                std::cout << "Phone: ";
                std::getline(std::cin, phone);
            }
            send(std::string(R"({"@type":"setAuthenticationPhoneNumber","phone_number":")") + jsonEscape(phone) + R"(","@extra":"phone"})");
        } else if (contains(*event, "authorizationStateWaitCode")) {
            std::string code;
            std::cout << "Code: ";
            std::getline(std::cin, code);
            send(std::string(R"({"@type":"checkAuthenticationCode","code":")") + jsonEscape(code) + R"(","@extra":"code"})");
        } else if (contains(*event, "authorizationStateWaitPassword")) {
            std::string password;
            std::cout << "2FA password: ";
            std::getline(std::cin, password);
            send(std::string(R"({"@type":"checkAuthenticationPassword","password":")") + jsonEscape(password) + R"(","@extra":"password"})");
        } else if (contains(*event, "authorizationStateReady")) {
            std::cout << "TDLib authorization ready\n";
            return 0;
        } else if (contains(*event, R"("@type":"error")")) {
            std::cerr << "TDLib error during login\n";
        }
    }
}

int TdClient::chats() {
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

int TdClient::sendText(std::int64_t chatId, const std::string& text) {
    send(textMessageJson(chatId, text));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
        auto event = receive(1.0);
        if (!event) continue;
        std::cout << *event << "\n";
        if (contains(*event, R"("@extra":"send")")) return 0;
    }
    return 1;
}

int TdClient::watch(std::optional<std::int64_t> chatId) {
    Bridge bridge;
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
                const auto text = event->substr(start, end - start);
                if (auto packet = bridge.tryParseTelegramText(text)) {
                    std::cout << "TGVerity packet: type=" << packet->packet.type << " body=" << packet->packet.body << "\n";
                }
            }
        }
    }
}

} // namespace tgverity
