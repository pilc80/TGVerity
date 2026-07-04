#include "tgverity/crypto_provider.h"

#include "tgverity/log.h"

#include <utility>

namespace tgverity {

namespace {

void announceInsecure() {
    auto& log = Logger::instance();
    if (log.announceInsecureMode()) {
        log.log(LogLevel::Warn, "crypto",
                "INSECURE MODE: IdentityCryptoProvider active. Bodies are NOT encrypted or "
                "authenticated. Prototype only — never ship as a release.");
    }
}

} // namespace

IdentityCryptoProvider::IdentityCryptoProvider() = default;

CryptoSuite IdentityCryptoProvider::suiteId() const {
    return CryptoSuite::Identity;
}

Bytes IdentityCryptoProvider::seal(const Bytes& plaintext, const Bytes& aad) const {
    (void)aad;
    announceInsecure();
    Logger::instance().log(LogLevel::Debug, "crypto",
                           "seal suite=identity bytes=" + std::to_string(plaintext.size()));
    return plaintext;
}

std::optional<Bytes> IdentityCryptoProvider::open(const Bytes& ciphertext, const Bytes& aad) const {
    (void)aad;
    announceInsecure();
    Logger::instance().log(LogLevel::Debug, "crypto",
                           "open suite=identity bytes=" + std::to_string(ciphertext.size()));
    return ciphertext;
}

} // namespace tgverity
