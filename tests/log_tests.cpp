#include "tgverity/log.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main() {
    using namespace tgverity;
    auto& log = Logger::instance();

    // Redaction helper.
    assert(redactSecret("hello-secret", true) == "[redact:12]");
    assert(redactSecret("hello-secret", false) == "hello-secret");
    log.setRedact(true);

    // setRedact toggle: in prototype builds, false disables redaction.
    // In non-prototype builds, it's forced true (no-op for false).
    log.setRedact(false);
    assert(redactSecret("hello-secret", log.redact()) ==
#if TGVERITY_PROTOTYPE_INSECURE
        "hello-secret"
#else
        "[redact:12]"
#endif
    );
    log.setRedact(true);
    assert(redactSecret("hello-secret", log.redact()) == "[redact:12]");
    assert(log.secureRedact() == true);

    // Level filtering: Trace < Debug < Info < Warn < Error.
    log.setLevel(LogLevel::Info);
    assert(static_cast<int>(log.level()) == static_cast<int>(LogLevel::Info));

    // File sink round-trip.
    const std::string path = "tgverity_log_tests.tmp";
    std::remove(path.c_str());
    log.setLevel(LogLevel::Trace);
    log.setFile(path);
    log.log(LogLevel::Info, "test", "hello-log");
    log.setFile(""); // close + flush

    std::ifstream in(path);
    assert(in.is_open());
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    assert(content.find("hello-log") != std::string::npos);
    assert(content.find("[test]") != std::string::npos);
    assert(content.find("INFO") != std::string::npos);
    std::remove(path.c_str());

    // INSECURE banner is one-shot: first call true, second false.
    log.setLevel(LogLevel::Trace);
    assert(log.announceInsecureMode() == true);
    assert(log.announceInsecureMode() == false);

    // Filtered level must not throw.
    log.setLevel(LogLevel::Error);
    log.log(LogLevel::Debug, "test", "filtered");
    log.log(LogLevel::Error, "test", "emitted");

    std::cout << "log_tests passed\n";
    return 0;
}
