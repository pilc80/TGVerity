# TGVerity Build Matrix — 2026-07-05 20:45 MSK

## Purpose

Define what “runs correctly” means per platform.

## Current matrix

| Platform | Status | Build path | Smoke target | Blockers |
|---|---|---|---|---|
| **macOS CLI** | ✅ working | CMake/Ninja | packet, P2P frame, tests | none known |
| **macOS TDLib CLI** | ✅ working | CMake/Ninja + TDLib | login/chats/send/watch | needs user credentials for real smoke |
| **Telegram Desktop macOS** | 🟡 prototype | tdesktop + branding patch + draft hook patch | branded app + normal chat + TGVerity relay | hooks unwired/draft; upstream pin needed |
| **Telegram Desktop Linux** | 🔴 not started | same Desktop source | same as macOS Desktop | build env + patch portability unknown |
| **Telegram Desktop Windows** | 🔴 not started | same Desktop source | same as macOS Desktop | build env + patch portability unknown |
| **Android** | 🔴 not started | Telegram Android fork | normal chat + verified Relay | hook map, adapter, QR/storage/notification work |
| **iOS** | 🔴 feasibility only | Telegram iOS fork or TDLib client | feasibility smoke | App Store, notification extension, storage, maintenance |
| **Web/headless** | 🔴 optional | MTProto library / web patch | packet send/watch | local security and UX constraints |

## Known verified commands

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure

cmake -S . -B build-tdlib -G Ninja -DTGVERITY_USE_TDLIB=ON -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build-tdlib
ctest --test-dir build-tdlib --output-on-failure
```

## Per-platform run criteria

| Platform | Minimum “runs correctly” |
|---|---|
| **CLI** | build/tests pass; `relay-pack`, `relay-parse`, `p2p-frame`, `selftest` work |
| **TDLib CLI** | TDLib version prints; login succeeds; chats list; normal send works; relay packet send/watch works |
| **Desktop** | branded TGVerity app launches; normal Telegram login/chat works; TGVerity packet path suppresses raw packet and renders virtual message |
| **Android** | normal chats untouched; TGVerity invite/verification/Relay works; raw notification suppressed |
| **iOS** | feasibility doc proves whether native fork or TDLib shell is viable |
| **Web/headless** | useful for testing only unless local-security constraints are solved |

## Desktop build policy

| Rule | Reason |
|---|---|
| Keep AppName branded as TGVerity | separate data path; avoids clobbering Telegram user data |
| Keep upstream AppVersionStr where server compatibility needs it | avoid Telegram compatibility breakage |
| Pin upstream commit before applying hook patch | reproducible patch/drift checks |
| Apply branding patch before hook patch | current docs expect disjoint edits except CMakeLists |
| Smoke normal Telegram before TGVerity | prove base client still works |

## Android build policy

Before Android code work:

1. Pin Telegram Android upstream commit.
2. Write hook map for send, id bind, incoming, delete, notification, render/edit suppression, QR/storage.
3. Define debug signing/package name/data path.
4. Define two-device or emulator+device smoke.
5. Add release blocker for hardcoded English/security strings.

## Cross-platform promotion rule

A platform moves from 🔴 to 🟡 when it has:

```text
pinned upstream/source
documented build steps
adapter hook map
first compile or known compile blocker
```

A platform moves from 🟡 to ✅ when it has:

```text
repeatable build
normal Telegram smoke
tgverity relay smoke
trust-label/notification leakage checks
recorded blockers = none critical
```

## Artifacts to avoid committing

```text
build*/
.tdlib/
local Telegram session data
Xcode derived data
packaged app binaries unless explicitly released
```
