# TGVerity Crypto Plan — 2026-07-05 20:45 MSK

## Current stance

`IdentityCryptoProvider` exists only to test packet/state/adapter flow. It is plaintext and insecure.

```text
v0.2: format-ready, crypto-insecure prototype
v0.3+: real cryptographic provider and protocol work
v1.0: reviewed, threat-model-bound security claims
```

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
| **C1 AEAD provider** | libsodium or equivalent AEAD | tamper/AAD/replay tests fail closed |
| **C2 Identity + verification** | long-term identity key + QR/safety number | `Safe` requires verified transcript |
| **C3 Async session setup** | X3DH-style or reviewed equivalent | first message can establish session over Relay |
| **C4 Message ratchet** | Double Ratchet-style or reviewed equivalent | forward secrecy + post-compromise recovery properties documented |
| **C5 Direct handshake** | Noise-style/WebRTC-secured channel | Direct has separate verified transport binding |
| **C6 External review** | protocol + implementation review | public security claims allowed only after fixes |

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
