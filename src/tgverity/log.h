#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

namespace tgverity {

enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
};

[[nodiscard]] const char* levelName(LogLevel level);

// Non-leaking representation of a secret for logs:
//   "[redact:<byte_len>]" when redact is true, else the raw string.
[[nodiscard]] std::string redactSecret(std::string_view secret, bool redact);

// Leveled logger with stderr sink + optional file sink + secret redaction.
class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level) { _level = level; }
    [[nodiscard]] LogLevel level() const { return _level; }

    // When TGVERITY_PROTOTYPE_INSECURE is set, this flag is user-tunable
    // (legacy prototype behaviour).  In non-prototype builds it is forced
    // to true and any call is a no-op — logged as ERROR if attempted.
    void setRedact(bool redact);

    // Secure redaction: always-on, cannot be disabled by the user.
    // Only available when !TGVERITY_PROTOTYPE_INSECURE.
    [[nodiscard]] bool secureRedact() const {
#if TGVERITY_PROTOTYPE_INSECURE
        (void)redact(); // suppress unused warning in prototype builds
        return redact();
#else
        return true;
#endif
    }

    [[nodiscard]] bool redact() const { return _redact; }

    // Open an append file sink. Empty path => stderr only and closes any open file.
    void setFile(const std::string& path);

    void log(LogLevel level, std::string_view component, std::string_view message);

    // One-time INSECURE MODE banner gate. Returns true only the first time it is called.
    bool announceInsecureMode();

private:
    Logger() = default;

    std::mutex _mutex;
    LogLevel _level = LogLevel::Info;
    bool _redact = true;
    bool _insecureBannerShown = false;
    std::string _filePath;
    std::ofstream _file;
};

} // namespace tgverity
