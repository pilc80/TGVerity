#include "tgverity/crypto_provider.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace tgverity;

    IdentityCryptoProvider crypto;
    assert(crypto.suiteId() == CryptoSuite::Identity);

    const Bytes aad = "aad-context";
    const Bytes plaintext = "secret body";

    const Bytes sealed = crypto.seal(plaintext, aad);
    assert(sealed == plaintext); // identity == plaintext

    const auto opened = crypto.open(sealed, aad);
    assert(opened.has_value());
    assert(*opened == plaintext);

    // Round-trip preserves bytes that look like envelope separators (no mangling).
    const Bytes tricky = "a=b\nc=d";
    const Bytes sealedTricky = crypto.seal(tricky, aad);
    assert(crypto.open(sealedTricky, aad).value() == tricky);

    std::cout << "crypto_provider_tests passed\n";
    return 0;
}
