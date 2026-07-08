#include "tgverity/tg_client.h"

#include "td/telegram/td_json_client.h"

#include "tgverity/log.h"
#include "app/config.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace tgverity {

namespace {

std::string jsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}

std::int64_t jsonGetInt(const std::string& json, std::size_t& pos) {
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'
           || json[pos] == '\n' || json[pos] == '\r')) ++pos;
    bool neg = false;
    if (pos < json.size() && json[pos] == '-') { neg = true; ++pos; }
    std::int64_t val = 0;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        val = val * 10 + (json[pos] - '0'); ++pos;
    }
    return neg ? -val : val;
}

std::string jsonGetString(const std::string& json, std::size_t& pos) {
    std::string out;
    if (pos >= json.size() || json[pos] != '"') return {};
    ++pos;
    while (pos < json.size()) {
        char c = json[pos++];
        if (c == '"') break;
        if (c == '\\') {
            if (pos < json.size()) {
                char esc = json[pos++];
                if      (esc == '"')  out += '"';
                else if (esc == '\\') out += '\\';
                else if (esc == 'n')  out += '\n';
                else if (esc == 't')  out += '\t';
                else if (esc == 'r')  out += '\r';
                else                  out += esc;
            }
        } else {
            out += c;
        }
    }
    return out;
}

} // namespace

class TdlibTelegramClient::Impl {
public:
    void* client = nullptr;
    bool connected = false;

    struct IdBoundEvent {
        std::string chatId;
        std::string correlationId;
        std::string serverId;
    };
    struct IncomingEvent {
        std::string chatId;
        std::string serverId;
        std::string text;
    };

    std::mutex eventMutex;
    std::condition_variable eventCv;
    std::vector<IdBoundEvent> idBoundEvents;
    std::vector<IncomingEvent> incomingEvents;

    std::function<void(const std::string&, const std::string&, const std::string&)> onMessageIdBoundCb;
    std::function<void(const std::string&, const std::string&, const std::string&)> onIncomingCb;

    std::atomic<bool> running{false};

    void connect() {
        if (connected) return;
        td_json_client_execute(nullptr,
            "{\"@type\":\"setLogVerbosityLevel\",\"new_verbosity_level\":0}");
        client = td_json_client_create();
        connected = true;
    }

    void close() {
        if (!client) return;
        td_json_client_send(client, "{\"@type\":\"close\",\"@extra\":\"close\"}");
        td_json_client_destroy(client);
        client = nullptr;
        connected = false;
    }

    int authenticate(const tgverity::AppConfig& config) {
        if (!connected) return -1;
        if (!config.hasTelegramCredentials()) {
            Logger::instance().log(LogLevel::Error, "tdlib", "no API credentials");
            return 2;
        }

        // Set TDLib parameters.
        const auto baseDir = config.tdlibDir.empty() ? ".tdlib" : config.tdlibDir;
        std::string params = "{\"@type\":\"setTdlibParameters\",\"use_test_dc\":false,\"database_directory\":\"";
        params += jsonEscape(baseDir);
        params += "/db\",\"files_directory\":\"";
        params += jsonEscape(baseDir);
        params += "/files\",\"database_encryption_key\":\"\",\"use_file_database\":true,"
                   "\"use_chat_info_database\":true,\"use_message_database\":true,"
                   "\"use_secret_chats\":false,\"api_id\":";
        params += config.apiId;
        params += ",\"api_hash\":\"";
        params += jsonEscape(config.apiHash);
        params += "\",\"system_language_code\":\"en\",\"device_model\":\"TGVerity CLI\","
                   "\"system_version\":\"macOS\",\"application_version\":\"0.2\"}";
        send(params);

        std::string phone = config.phone;

        for (int retry = 0; retry < 600; ++retry) {
            const char* raw = td_json_client_receive(client, 1.0);
            if (!raw) continue;

            // Parse @type.
            std::string json(raw);
            std::free(const_cast<char*>(raw));
            auto tp = json.find("\"@type\":");
            if (tp == std::string::npos) continue;
            tp += 8;
            std::string type = jsonGetString(json, tp);

            if (type == "authorizationStateWaitTdlibParameters") {
                send(params);
            } else if (type == "authorizationStateWaitEncryptionKey") {
                send("{\"@type\":\"checkDatabaseEncryptionKey\",\"encryption_key\":\"\"}");
            } else if (type == "authorizationStateWaitPhoneNumber") {
                if (phone.empty()) {
                    std::cout << "Phone: ";
                    if (!std::getline(std::cin, phone) || phone.empty()) {
                        return 2;
                    }
                }
                send("{\"@type\":\"setAuthenticationPhoneNumber\",\"phone_number\":\""
                     + jsonEscape(phone) + "\",\"@extra\":\"phone\"}");
            } else if (type == "authorizationStateWaitCode") {
                std::string code;
                std::cout << "Code: ";
                if (!std::getline(std::cin, code) || code.empty()) {
                    return 2;
                }
                send("{\"@type\":\"checkAuthenticationCode\",\"code\":\""
                     + jsonEscape(code) + "\",\"@extra\":\"code\"}");
            } else if (type == "authorizationStateWaitPassword") {
                std::string password;
                std::cout << "2FA password: ";
                if (!std::getline(std::cin, password) || password.empty()) {
                    return 2;
                }
                send("{\"@type\":\"checkAuthenticationPassword\",\"password\":\""
                     + jsonEscape(password) + "\",\"@extra\":\"password\"}");
            } else if (type == "authorizationStateReady") {
                std::cout << "TDLib auth ready\n";
                return 0;
            } else if (type == "error") {
                std::cerr << "Auth error\n";
                return 1;
            }
        }
        return 1;
    }

    void send(const std::string& json) {
        if (client) td_json_client_send(client, json.c_str());
    }

    void processEvent(const std::string& json) {
        auto typePos = json.find("\"@type\":");
        if (typePos == std::string::npos) return;
        typePos += 8;
        std::string type = jsonGetString(json, typePos);

        if (type == "ok" || type == "error") return;

        // updateMessageSendSucceeded
        if (type == "updateMessageSendSucceeded") {
            auto extraPos = json.find("\"@extra\":");
            if (extraPos != std::string::npos) {
                extraPos += 9;
                std::string corrId = jsonGetString(json, extraPos);

                auto msgPos = json.find("\"message\":");
                if (msgPos != std::string::npos) {
                    msgPos += 10;
                    auto chatPos = json.find("\"chat_id\":", msgPos);
                    if (chatPos != std::string::npos) {
                        chatPos += 11;
                        std::int64_t chatIdInt = jsonGetInt(json, chatPos);
                        std::string chatIdStr = std::to_string(chatIdInt);

                        auto idPos = msgPos + json.find("\"id\":", msgPos);
                        if (idPos != std::string::npos) {
                            idPos += 5;
                            std::int64_t srvIdInt = jsonGetInt(json, idPos);
                            std::string srvIdStr = std::to_string(srvIdInt);

                            std::lock_guard<std::mutex> lock(eventMutex);
                            idBoundEvents.push_back({chatIdStr, corrId, srvIdStr});
                            eventCv.notify_one();

                            // Dispatch immediately if callback set.
                            if (onMessageIdBoundCb) {
                                auto evt = idBoundEvents.back();
                                idBoundEvents.pop_back();
                                onMessageIdBoundCb(evt.chatId, evt.correlationId, evt.serverId);
                            }
                        }
                    }
                }
            }
            return;
        }

        // updateNewMessage
        if (type == "updateNewMessage") {
            auto msgPos = json.find("\"message\":");
            if (msgPos != std::string::npos) {
                msgPos += 10;

                auto idPos = msgPos + json.find("\"id\":", msgPos);
                if (idPos != std::string::npos) {
                    idPos += 5;
                    std::int64_t srvIdInt = jsonGetInt(json, idPos);
                    std::string srvIdStr = std::to_string(srvIdInt);

                    auto chatPos = msgPos + json.find("\"chat_id\":", msgPos);
                    if (chatPos != std::string::npos) {
                        chatPos += 11;
                        std::int64_t chatIdInt = jsonGetInt(json, chatPos);
                        std::string chatIdStr = std::to_string(chatIdInt);

                        std::string text;
                        auto fmtPos = msgPos + json.find("formattedText", msgPos);
                        if (fmtPos != std::string::npos) {
                            auto textValPos = msgPos + json.find("\"text\":", fmtPos);
                            if (textValPos != std::string::npos) {
                                textValPos += 7;
                                text = jsonGetString(json, textValPos);
                            }
                        }

                        if (!text.empty()) {
                            std::lock_guard<std::mutex> lock(eventMutex);
                            incomingEvents.push_back({chatIdStr, srvIdStr, text});
                            eventCv.notify_one();

                            // Dispatch immediately if callback set.
                            if (onIncomingCb) {
                                auto evt = incomingEvents.back();
                                incomingEvents.pop_back();
                                onIncomingCb(evt.chatId, evt.serverId, evt.text);
                            }
                        }
                    }
                }
            }
            return;
        }
    }

    void receiveLoop() {
        while (running.load()) {
            auto raw = td_json_client_receive(client, 0.05);
            if (raw) processEvent(raw);
        }
    }

    void startReceiveLoop() {
        if (running.exchange(true)) return;
        std::thread([this]() { receiveLoop(); }).detach();
    }

    // Wait for an event matching correlationId. Returns true if found.
    bool waitForIdBound(const std::string& corrId) {
        std::unique_lock<std::mutex> lock(eventMutex);
        size_t before = idBoundEvents.size();
        eventCv.wait_for(lock, std::chrono::seconds(30), [this, &before] {
            return idBoundEvents.size() > before;
        });
        // Check if the first new event matches our correlationId.
        for (size_t i = before; i < idBoundEvents.size(); ++i) {
            if (idBoundEvents[i].correlationId == corrId) {
                // Keep it for later processing by setListener.
                return true;
            }
        }
        return false;
    }
};

TdlibTelegramClient::TdlibTelegramClient() {
    _impl = std::make_unique<Impl>();
    _impl->connect();
}

int TdlibTelegramClient::authenticate() {
    if (!_impl) return -1;
    return _impl->authenticate(tgverity::AppConfig::fromEnvironmentOrPrompt());
}

int TdlibTelegramClient::authenticate(const tgverity::AppConfig& config) {
    if (!_impl) return -1;
    return _impl->authenticate(config);
}

TdlibTelegramClient::~TdlibTelegramClient() {
    if (_impl) {
        _impl->running = false;
        _impl->close();
    }
}

std::string TdlibTelegramClient::sendPacketText(const std::string& chatId,
                                                const std::string& correlationId,
                                                const std::string& packetText,
                                                PacketSendOptions options) {
    if (!options.disableLinkPreview || options.allowEntities) {
        Logger::instance().log(LogLevel::Warn, "tdlib", "unsafe packet send options rejected");
        return {};
    }
    if (!_impl->client || !_impl->connected) return {};
    if (!_impl->running.load()) {
        _impl->startReceiveLoop();
    }

    // Send the message.
    std::ostringstream json;
    json << "{\"@type\":\"sendMessage\",\"chat_id\":" << chatId
         << ",\"input_message_content\":{\"@type\":\"inputMessageText\",\"disable_web_page_preview\":true,\"text\":{\"@type\":\"formattedText\",\"text\":\"";
    json << jsonEscape(packetText);
    json << "\",\"entities\":[]},\"@extra\":\"" << correlationId << "\"}";
    td_json_client_send(_impl->client, json.str().c_str());

    return correlationId;
}

void TdlibTelegramClient::deleteMessagesRevoke(const std::string& chatId,
                                               const std::vector<std::string>& serverIds) {
    if (!_impl->client || !_impl->connected || serverIds.empty()) return;
    std::ostringstream json;
    json << "{\"@type\":\"deleteMessages\",\"chat_id\":" << chatId
         << ",\"revoke\":false,\"message_ids\":[";
    for (size_t i = 0; i < serverIds.size(); ++i) {
        try {
            if (i > 0) json << ",";
            json << std::stoll(serverIds[i]);
        } catch (...) {
            Logger::instance().log(LogLevel::Warn, "tdlib",
                                   "skip undeletable serverId=" + serverIds[i]);
        }
    }
    json << "]}";
    td_json_client_send(_impl->client, json.str().c_str());
    Logger::instance().log(LogLevel::Info, "tdlib",
                           "deleteMessagesRevoke chat=" + chatId + " count=" + std::to_string(serverIds.size()));
}

void TdlibTelegramClient::suppressRawRender(const std::string&) {
    // No-op: CLI mode has no raw UI to suppress.
}

void TdlibTelegramClient::suppressRawNotification(const std::string&) {
}

void TdlibTelegramClient::suppressRawEdit(const std::string&) {
}

void TdlibTelegramClient::renderVirtualMessage(const std::string& chatId, const VirtualMessage& msg) {
    std::cout << "[TGVerity] chat=" << chatId << " packet=" << msg.packetId
              << " plaintext=" << msg.plaintext << "\n";
}

void TdlibTelegramClient::setListener(AdapterListener* listener) {
    if (!_impl) return;

    _impl->onMessageIdBoundCb = [listener](const std::string& chatId,
                                            const std::string& corrId,
                                            const std::string& srvId) {
        if (listener) listener->onMessageIdBound(chatId, corrId, srvId);
    };
    _impl->onIncomingCb = [listener](const std::string& chatId,
                                      const std::string& srvId,
                                      const std::string& text) {
        if (listener) listener->onIncomingMessage(chatId, srvId, text);
    };

    // Process events that accumulated before setListener was called.
    std::lock_guard<std::mutex> lock(_impl->eventMutex);
    for (auto& evt : _impl->idBoundEvents) {
        if (listener) listener->onMessageIdBound(evt.chatId, evt.correlationId, evt.serverId);
    }
    for (auto& evt : _impl->incomingEvents) {
        if (listener) listener->onIncomingMessage(evt.chatId, evt.serverId, evt.text);
    }
    _impl->idBoundEvents.clear();
    _impl->incomingEvents.clear();
}

} // namespace tgverity