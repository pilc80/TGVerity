# TGVerity Crypto Plan — 2026-07-06 15:16 MSK

## Current stance

`IdentityCryptoProvider` exists only to test packet/state/adapter flow. It is plaintext and insecure.

```text
v0.2: format-ready, crypto-insecure prototype
v0.3+: real cryptographic provider and protocol work
v1.0: reviewed, threat-model-bound security claims
```

## Desktop two-client security bar

Two Telegram Desktop users may be called ready for security-gated internal testing only when:

1. `IdentityCryptoProvider` is impossible in secure mode.
2. Each Desktop client has a persistent TGVerity device identity key.
3. In-chat QR verification binds both Telegram accounts, both identity keys, both device ids, chat/peer id, protocol version, crypto/session suite, challenge/expiry, and negotiated parameter hash.
4. `Safe` appears only after transcript verification.
5. AAD binds packet type/id/counter, sender/receiver identity ids, Telegram account ids, chat/peer id, device ids, protocol version, crypto suite, and session id/challenge.
6. Replay cache survives restart.
7. Key change removes `Safe` and blocks silent accept.
8. Logs cannot contain plaintext, carrier body, private keys, session keys, or safety secrets.

## Recommended implementation track

```text
C1 alpha: libsodium-backed AEAD + verified identity/session transcript
C2 reviewed MVP: X3DH-style async setup + Double Ratchet-style messaging, or vetted equivalent
```

C1 is acceptable only for internal two-client Desktop proof. Public security claims require reviewed C2+ design and external review.

## Security goals

| Goal | Required mechanism |
|---|---|
| Telegram cannot read TGVerity content | encrypt before Telegram Relay |
| MITM/key substitution is detectable | QR/safety-number verification transcript |
| Packet tamper is rejected | AEAD authentication over packet + context |
| Replay/copy-to-chat is rejected | packet counters + AAD binding to chat/session/account/device |
| Key changes are visible | persistent identity key + key-change state |
| No custom primitives | reviewed libraries/protocols only |

## Milestones

| Milestone | Scope | Done when |
|---|---|---|
| **C0 Prototype seam** | current provider interface | identity provider warns loudly; tests pass |
| **C1 AEAD provider** | libsodium XChaCha20-Poly1305 AEAD; nonce management spec; HKDF KDF spec; memory zeroization policy; secure storage adapter interface | tamper/AAD/replay tests fail closed; nonce never repeats under restart |
| **C2 Identity + verification** | long-term identity key (X25519); QR/safety number wire format; transcript binding | `Safe` requires verified transcript; key-change detection trigger fires |
| **C3 Async session setup** | X3DH handshake (4 DH, HKDF master secret); prekey bundle format/wire; KeyExchangeMessage; session state machine (named/keyed/indexed); nonce spec applied; prekey bundle validation (signature + cross-key-binding) | first encrypted message can establish session over Relay; 1:K consumed and exhausted notification works |
| **C4 Message ratchet** | Symmetric ratchet (chain key advance); Asymmetric ratchet (ephemeral DH); per-message key derivation; MAC tag verification before decryption; counter overflow at 2^63 | forward secrecy + post-compromise recovery properties documented; ratchet state persists across restart |
| **C5 Direct handshake** | Noise-style/WebRTC-secured channel | Direct has separate verified transport binding; QUIC/WebRTC ICE/STUN tested |
| **C6 External review** | protocol + implementation review; formal analysis (ProVerif/Tamarin) if budget allows | public security claims allowed only after fixes |

## Provider rules

| Provider | Build use | Release use |
|---|---|---|
| `IdentityCryptoProvider` | tests/prototype only | ❌ blocked |
| `AeadCryptoProvider` | alpha once added | 🟡 internal only until verification/session design complete |
| ratcheted provider | beta | 🟡 external review required |
| reviewed provider/protocol | release | ✅ if release gates pass |

## AAD binding

Every encrypted packet must authenticate at least:

```text
TGVerity protocol name/version
crypto suite id
packet type
packet id / counter
sender TGVerity identity key id
receiver TGVerity identity key id
Telegram account ids involved
Telegram chat/peer id
device ids
session id / challenge
```

Exact fields may evolve, but omission must be explicit in the protocol spec.

## Verification transcript

A QR/safety-number transcript must bind:

```text
both Telegram accounts
both TGVerity identity public keys
both device ids
protocol name/version
crypto suite/session suite
session challenge/expiry
hash of negotiated session parameters
```

A session is **Unverified** until this transcript is verified.

## Key-change policy

| Event | Required behavior |
|---|---|
| known peer identity changes | enter `tgverity_key_changed`; block Safe |
| new device appears | show as unverified device |
| local identity reset | warn user; peers must reverify |
| lost device | no silent recovery; recovery design required later |

## Storage requirements

| Data | Storage rule |
|---|---|
| identity private key | platform secure storage where available |
| session state | encrypted local store; never Telegram cloud plaintext |
| replay cache/counters | durable enough to survive restart |
| verification state | durable and auditable to user |
| logs | redacted; no plaintext/keys/safety secrets |

## Testing gates

| Gate | Must prove |
|---|---|
| tamper test | bit flip rejects packet |
| AAD mismatch | copy to another chat/session rejects packet |
| replay test | old packet rejected or ignored safely |
| key-change test | Safe removed and reverify required |
| insecure-build test | release build fails if IdentityCryptoProvider is active |
| log-redaction test | no plaintext/key material in logs |

## Allowed claims by milestone

| Milestone | Allowed public claim |
|---|---|
| C0 | “prototype; not secure” |
| C1 | “encrypted packet prototype in tests” |
| C2-C4 | “alpha; cryptographic design under review” |
| C6 | “protects message content from Telegram Relay under the published threat model” |

## Explicit non-goals for MVP crypto

1. Group messaging.
2. Plausible deniability claims.
3. Metadata privacy in Telegram Relay.
4. Anonymous account creation.
5. Protection from compromised endpoints.
6. Post-quantum migration before external review (Signal's PQXDH is noted for future planning).

## Critical missing designs (before C3)

| Design | Required content | Blocks |
|---|---|---|
| **Nonce management spec** | Per-message unique nonce; counter-based; 24-byte nonce for XChaCha20-Poly1305; never repeat | C1 AEAD provider |
| **Key derivation spec** | HKDF per RFC 5869; distinct `info` contexts for session keys, chain keys, MAC keys | C1–C4 |
| **X3DH wire format** | Prekey bundle format, X3DH handshake transcript, KeyExchangeMessage envelope, AEAD AD binding | C3 |
| **Session state machine** | Named sessions, keyed sessions, indexed prekeys (Signal session layer spec) | C3 |
| **Prekey bundle validation** | Signature verification of signed prekey; cross-key-binding check | C3 |
| **Ratchet state layout** | Root key, sending chain key, receiving chain key, per-message MAC key derivation | C4 |
| **Memory zeroization policy** | `sodium_memzero()` on all secret buffers post-use; `secure_alloc`/`secure_realloc` | C1 |
