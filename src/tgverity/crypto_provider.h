#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace tgverity {

// Crypto suite identifiers persisted in the packet envelope (crypto_suite field).
enum class CryptoSuite : std::uint16_t {
    Identity = 0,            // INSECURE: no encryption, no authentication. Prototype only.
    // XChaCha20Poly1305Ietf = 1  // future libsodium-backed provider
};

// Raw bytes carried as std::string.
using Bytes = std::string;

// Crypto seam. v0.2 ships only IdentityCryptoProvider (plaintext, insecure).
// A libsodium-backed provider implements the same interface later with zero
// packet-format change (the suite id travels in every packet).
class CryptoProvider {
public:
    virtual ~CryptoProvider() = default;
    [[nodiscard]] virtual CryptoSuite suiteId() const = 0;
    [[nodiscard]] virtual Bytes seal(const Bytes& plaintext, const Bytes& aad) const = 0;
    [[nodiscard]] virtual std::optional<Bytes> open(const Bytes& ciphertext, const Bytes& aad) const = 0;
};

// No-op provider: returns input unchanged. Logs an INSECURE MODE banner once.
class IdentityCryptoProvider final : public CryptoProvider {
public:
    IdentityCryptoProvider();
    [[nodiscard]] CryptoSuite suiteId() const override;
    [[nodiscard]] Bytes seal(const Bytes& plaintext, const Bytes& aad) const override;
    [[nodiscard]] std::optional<Bytes> open(const Bytes& ciphertext, const Bytes& aad) const override;
};

} // namespace tgverity
