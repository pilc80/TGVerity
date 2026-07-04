#include "tgverity/log.h"

#include <cstdio>
#include <ctime>
#include <utility>

namespace tgverity {

const char* levelName(LogLevel level) {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "?";
}

std::string redactSecret(std::string_view secret, bool redact) {
    if (!redact) return std::string(secret);
    return "[redact:" + std::to_string(secret.size()) + "]";
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(_mutex);
    _filePath = path;
    if (_file.is_open()) {
        _file.flush();
        _file.close();
    }
    if (!path.empty()) {
        _file.open(path, std::ios::out | std::ios::app);
    }
}

void Logger::log(LogLevel level, std::string_view component, std::string_view message) {
    if (static_cast<int>(level) < static_cast<int>(_level)) {
        return;
    }
    std::lock_guard<std::mutex> lock(_mutex);

    std::time_t now = std::time(nullptr);
    char ts[24];
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    std::string line = std::string(ts) + " " + levelName(level) + " ["
        + std::string(component) + "] " + std::string(message) + "\n";

    std::fwrite(line.data(), 1, line.size(), stderr);
    if (_file.is_open()) {
        _file.write(line.data(), static_cast<std::streamsize>(line.size()));
        _file.flush();
    }
}

bool Logger::announceInsecureMode() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_insecureBannerShown) {
        return false;
    }
    _insecureBannerShown = true;
    return true;
}

} // namespace tgverity
