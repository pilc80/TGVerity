# TGVerity Testing Plan — 2026-07-05 20:45 MSK

## Goal

Prove TGVerity works without weakening Telegram compatibility or security-state honesty.

```text
Unit tests prove core rules.
Integration tests prove adapter flow.
E2E smokes prove real Telegram/client behavior.
Release gates block unsafe claims/builds.
```

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
| real crypto | AEAD seal/open, tamper, AAD mismatch, replay |
| release safety | build/test fails if insecure provider is enabled for release |
| TDLib real flow | two-account send/watch relay smoke |
| Desktop GUI | raw packet hidden, virtual message rendered, normal chat unchanged |
| Android | notification suppression, QR verification, storage, normal chat unchanged |
| search/history leakage | plaintext absent from Telegram search/raw message store |
| key change | Safe removed, reverify required |
| upstream drift | patches apply to pinned upstream commit |

## Core test gates

```sh
cmake --build build
ctest --test-dir build --output-on-failure

cmake --build build-tdlib
ctest --test-dir build-tdlib --output-on-failure
```

## Required security tests

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
verify edit affordance hidden for TGVerity packet/message
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
