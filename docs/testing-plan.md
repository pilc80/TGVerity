# TGVerity Testing Plan — 2026-07-06 15:16 MSK

## Goal

Prove TGVerity works without weakening Telegram compatibility or security-state honesty.

```text
Unit tests prove core rules.
Integration tests prove adapter flow.
E2E smokes prove real Telegram/client behavior.
Release gates block unsafe claims/builds.
```

## Desktop two-client security gate

Before claiming two Desktop users can test security-gated TGVerity Relay:

```text
secure-mode core tests pass
TDLib two-account transport smoke passes
Desktop hook patch applies to pinned upstream
TGVerity.app builds and launches with TGVerity data path
normal Telegram chat works unchanged
in-chat QR verification reaches Safe Relay
TGVerity sentinel message decrypts only in virtual TGVerity UI
raw carrier bubble and raw notification are suppressed
Known accessible Telegram search/history/raw surfaces do not contain sentinel plaintext
logs contain no sentinel, packet body, keys, safety number, or session secret
key-change simulation removes Safe and requires reverify
```

TDLib two-account transport smoke is a planned gate until `TdlibTelegramClient` implements the `TelegramAdapter` path.

Use a fresh sentinel per run: `TGV_LEAK_SENTINEL_<run_id>`.

## Test layers

| Layer | Scope | Required before |
|---|---|---|
| **Unit** | packet codec, crypto provider, state machine, store, logging | every commit |
| **Core integration** | bridge + fake Telegram client send/receive/ACK/cleanup | every commit |
| **TDLib integration** | login/chats/send/watch through real TDLib | prototype release |
| **Desktop E2E** | patched app normal chat + TGVerity relay path | Desktop proof |
| **Android E2E** | two devices/accounts verified Relay | Android MVP |
| **Security regression** | no plaintext/log/search/notification leaks | any public build |
| **Upstream drift** | patch applies/builds against pinned upstream | fork releases |

## Current covered tests

| Area | Status |
|---|---:|
| relay packet codec | ✅ |
| bridge fake-client flow | ✅ |
| logging/redaction basics | ✅ |
| crypto provider seam | ✅ prototype |
| state machine | ✅ |
| session store | ✅ |
| TDLib compile path | ✅ |

## Missing tests

| Gap | Required test |
|---|---|
| real crypto | AEAD seal/open, tamper, AAD mismatch, replay, restart replay cache |
| QR verification | transcript bind, safety number mismatch, expiry, wrong chat/account rejection |
| release safety | build/test fails if insecure provider is enabled for secure mode |
| TDLib real flow | two-account send/watch relay smoke |
| Desktop GUI | raw packet hidden, virtual message rendered, normal chat unchanged |
| Android | notification suppression, QR verification, storage, normal chat unchanged |
| search/history leakage | plaintext absent from Telegram search/raw message store |
| key change | Safe removed, reverify required |
| upstream drift | patches apply to pinned upstream commit |

> New from code review 2026-07-07 (see `docs/code-review-findings.md`):

| Gap | Required test |
|---|---|
| ACK suppression | ACK packets not rendered/notified on raw Telegram path |
| log redaction enforced | `setRedact(false)` has no effect when `!TGVERITY_PROTOTYPE_INSECURE` |
| session persistence | replay cache and counters survive process restart |
| plaintext wipe | bridge does not retain plaintext after cleanup_done |
| parser strictness | empty body rejected for relay_text in both core and shim |
| AAD binding completeness | AEAD rejects when account/device/session fields missing from AAD |
| X3DH handshake | prekey bundle validation, signature check, cross-key-binding, KeyExchangeMessage round-trip |
| Ratchet state | root key derivation, chain key advance, per-message key derivation, counter overflow at 2^63 |
| Nonce uniqueness | same nonce never used twice under any restart/replay scenario |
| Memory zeroization | secret buffers cleared after use; crash/dump would not expose keys |
| Shim/core format parity | base32 envelope round-trips through both modules with identical output |

## Core test gates

```sh
cmake --build .build/core
ctest --test-dir .build/core --output-on-failure

cmake --build .build/core-tdlib
ctest --test-dir .build/core-tdlib --output-on-failure
```

## Required security tests

> Updated from code review 2026-07-07.

| Test | Expected result |
|---|---|
| bit-flip packet body | decrypt/auth fails; no plaintext rendered |
| wrong chat/session AAD | packet rejected |
| replay packet | ignored or flagged; no duplicate trusted message |
| fake prefix text | never trusted without valid envelope/auth |
| key changed | `Safe` removed; reverify shown |
| raw packet notification | suppressed |
| log scan | no plaintext, keys, safety numbers, or packet bodies |
| release build with identity crypto | fails |
| ACK packet suppress | `suppressRawRender`/`suppressRawNotification`/`suppressRawEdit` called |
| log redaction enforced | `Logger::setRedact(false)` no-ops when `!TGVERITY_PROTOTYPE_INSECURE` |
| session persistence | replay cache and counters survive process restart |
| plaintext wipe | bridge releases plaintext after cleanup_done |
| parser strictness | empty body rejected for relay_text |
| AAD binding completeness | packet rejected when account/device/session missing from AAD |

## Platform E2E scenarios

### TDLib CLI

```text
login
list chats
send normal message
send TGVerity relay packet
watch peer updates
parse incoming packet
verify normal messages unchanged
```

### Desktop

```text
launch branded TGVerity app
login without affecting Telegram.app data
send normal message
send TGVerity message in enabled chat
verify raw packet bubble suppressed
verify raw packet notification suppressed
verify virtual TGVerity bubble/status shown
verify in-chat QR/safety verification changes state to Safe only after transcript validation
verify edit affordance hidden for TGVerity packet/message
verify known accessible Telegram search/history/raw surfaces do not contain leak sentinel
verify key change removes Safe and requires reverify
```

### Android

```text
install debug TGVerity package
login test account
normal chat send/receive
TGVerity invite
QR/safety verification
verified Relay send/receive
raw notification suppressed
key-change warning smoke
```

## CI plan

| Stage | Runs |
|---|---|
| **core** | CMake build + unit/integration tests |
| **tdlib** | TDLib-enabled build if dependency available |
| **lint/docs** | markdown link check + no generated/session artifacts |
| **desktop patch** | apply patches to pinned tdesktop commit; compile shim; optional GUI build |
| **security grep** | block forbidden claims/log strings in release mode |

## Release gates

A release is blocked if:

```text
any core test fails
IdentityCryptoProvider is active in release build
Safe can appear before verification
raw Telegram path can receive TGVerity plaintext
raw packet notification is not suppressed on target platform
key change does not force reverify
patch drift is unresolved
README/security docs claim more than tests prove
```

## Manual evidence format

For each platform smoke, record:

```text
platform / OS / device
commit hashes: TGVerity + upstream Telegram
build command
account setup: test accounts only
normal Telegram result
TGVerity Relay result
notification/search leakage result
known failures
```

## Completion definition

TGVerity is not “done” because tests pass. It is done for a platform only when:

```text
normal Telegram smoke passes
TGVerity verified Relay smoke passes
trust labels are correct
security regression checks pass
docs match observed behavior
release blockers are clear
```
