# TGVerity

**Unofficial Telegram-compatible client with verified direct/relay secure sessions.**

**Status:** concept draft  
**Primary platform:** Android first  
**Core rule:** normal Telegram behavior stays intact; TGVerity adds an optional verified secure-session layer.

---

## Product Promise

TGVerity lets two TGVerity users create a separately verified encrypted session inside a Telegram-compatible client.

```text
Normal Telegram:     Alice → Telegram servers → Bob
TGVerity Direct:     Alice ↔ Bob
TGVerity Relay:      Alice → Telegram carrying opaque TGVerity packets → Bob
```

TGVerity must never silently downgrade a secure session to normal Telegram chat.

---

## Core Principles

```text
Normal Telegram must remain normal.
TGVerity must remain isolated from Telegram-side code.
Safe means identity-verified.
Direct transport is best-effort.
Relay transport is explicit and honestly labeled.
No hidden Telegram transport is assumed.
No custom cryptographic primitives.
```

---

## Real UI Concept

TGVerity should feel native to Telegram and add the smallest possible UI surface.

| Surface | TGVerity behavior |
|---|---|
| **App icon** | Telegram-style icon with a small same-style shield mark; avoid cluttering the paper plane. |
| **Chat list** | Show a compact shield after the user name, or after Telegram Premium star if present. |
| **Dialog header/subtitle** | Primary persistent state: `TGVerity Safe Direct`, `TGVerity Safe Relay`, `TGVerity Encrypted — Unverified`, etc. |
| **Pinned-style bar** | Use only for action/risk states: verify, key changed, direct failed, relay choice. |
| **QR overlay** | Show QR code, camera scan button, and safety-number fallback. |
| **Message bubbles** | Mark TGVerity-protected messages with a minimal green shield/tick badge only where needed to avoid clutter. |
| **Accessibility** | Never rely on color alone; every shield/status needs text and accessible label. |

### Shield States

| State | Indicator |
|---|---|
| No TGVerity peer | no shield |
| TGVerity peer, not verified | gray shield + blue `!` |
| Verified relay | colored shield + relay label |
| Verified direct | colored shield + direct label |
| Key changed | warning shield + `reverify` action |

### Pinned-Style Bar Rule

TGVerity should reuse Telegram's pinned-message visual style at runtime if possible, so style updates follow upstream Telegram automatically.

This needs code research later:

1. Prefer surgical component/style reuse over copying style constants.
2. Avoid fragile intersection with the real pinned-message bar.
3. If both pinned content and TGVerity status/action exist, define stacking or merging rules before implementation.
4. Do not invent a new permanent banner unless reuse is not safely possible.

---

## Trust States

A session must not be labeled **Safe** until the TGVerity identity key is verified.

| Internal state | User label | Meaning |
|---|---|---|
| `normal_cloud` | Telegram Cloud | Normal Telegram chat. |
| `normal_secret` | Telegram Secret | Telegram-native secret chat, not TGVerity. |
| `tgverity_detected` | TGVerity Available | Peer appears to support TGVerity. |
| `tgverity_unverified` | TGVerity Encrypted — Unverified | Encrypted, but MITM risk remains. |
| `tgverity_safe_relay` | TGVerity Safe Relay | Identity verified; Telegram carries opaque packets. |
| `tgverity_safe_direct` | TGVerity Safe Direct | Identity verified; direct transport active. |
| `tgverity_direct_failed` | Direct unavailable | P2P failed; user may choose Relay or Cancel. |
| `tgverity_key_changed` | Key changed — reverify | Previous identity changed; do not call Safe. |

Avoid vague labels such as `Secure`, `Private`, `Protected`, `Military-grade`, or `Serverless` unless the exact mode justifies them.

---

## Discovery

MVP discovery is explicit only.

```text
User chooses peer → taps TGVerity invite → confirms Send → one visible text invite is sent
```

Rules:

1. No silent background discovery probes.
2. No hidden discovery messages.
3. No broad contact scanning for MVP.
4. The invite must be understandable in unsupported Telegram clients.
5. QR can be used for in-person discovery/verification.
6. Profile/bio markers are optional later signals only, not required trust input.

Example invite shape:

```text
TGVerity invite. Open with TGVerity to establish a safe connection.
v1:<invite-payload>
```

---

## Verification Flow

Default UX:

```text
Open TGVerity-capable dialog
  ↓
Show gray shield + "Establish safe connection"
  ↓
User opens QR overlay
  ↓
Scan QR or compare safety number
  ↓
Verified identity → Safe Relay or Safe Direct label may be shown
```

One QR scan can be enough only if the protocol transcript binds both sides. README does not define final crypto details yet.

Minimum verification binding:

```text
both Telegram accounts
both TGVerity identity public keys
both device ids
TGVerity protocol name/version
session challenge/expiry
cryptographic confirmation
```

Mutual scan may be added as optional hardening. Safety-number comparison remains the fallback.

---

## Transport Modes

### Normal Telegram

Used when TGVerity is unavailable or the user sends a normal Telegram message.

```text
Alice → Telegram servers → Bob
```

### TGVerity Relay

MVP transport. Telegram carries opaque TGVerity packets through ordinary Telegram-visible text messages only.

```text
Plaintext → TGVerity encrypts → opaque packet → Telegram text message → peer TGVerity decrypts
```

TGVerity must not rely on hidden Telegram client-to-client channels.

| Carrier | Use | Caveat |
|---|---|---|
| Text message | **MVP only carrier** for small packets/control messages | May appear as opaque packet text in unsupported clients. |
| Generic document/file | Later research for larger encrypted payloads | Visible file message; filename/MIME/size metadata need minimization. |
| Photo/video media | Avoid for exact packets | Telegram may transform, thumbnail, compress, or process. |
| Bot/WebApp data | Not core transport | Bot-mediated, not user-to-user E2EE transport. |

TGVerity clients may hide or render relay packets locally, but official/unsupported Telegram clients may still show packet text or encrypted blob files.

Telegram text relay is not a raw socket. Normal Telegram rules may reject, delay, rate-limit, delete, edit, or visually transform packet messages.

MVP packet rules:

1. Text-only relay.
2. Short packet prefix understandable in unsupported clients.
3. Safe encoding alphabet: avoid URL-like, command-like, mention-like, hashtag-like, email-like strings.
4. Disable link previews where the Telegram API allows it.
5. Do not use Markdown/HTML/entities for packet text.
6. Store Telegram `random_id` and server `msg_id` for each logical packet.
7. Reuse the same `random_id` when retrying the same packet.
8. Detect packet message edits/deletes; reject altered packets cryptographically.
9. Queue/back off on Telegram flood, slow-mode, privacy, or send-forbidden errors.
10. Show honest status: queued, delayed, failed, sent, received.

Unsupported-client packet text should be explanatory, e.g.:

```text
TGVerity secure packet. Open with TGVerity to read.
v1:<safe-encoded-payload>
```

Telegram may still observe:

```text
sender / recipient
timing / frequency
approximate packet size
delivery behavior
account linkage
stored relay history
```

### TGVerity Direct

Direct P2P is not a product requirement for MVP, but v0.1 should test both relay and P2P protocol feasibility as early technical prototypes.

```text
Alice TGVerity ↔ Bob TGVerity
```

On entering a secured chat, TGVerity may try P2P for a short timeout, e.g. 5 seconds. If direct connection fails, the user chooses **Telegram Relay** or **Cancel**.

```text
Try Direct
  ├─ success → Safe Direct / Unverified Direct
  └─ timeout/fail → ask: Relay or Cancel
```

No silent fallback to normal Telegram chat.

---

## Protocol Direction

Protocol details need a dedicated reviewed spec before implementation claims.

Recommended direction:

| Need | Candidate |
|---|---|
| Async 1:1 setup over relay | X3DH-style setup |
| Message encryption | Double Ratchet-style session |
| Direct channel handshake | Noise-style handshake or WebRTC-secured channel |
| Manual verification | QR + safety number |
| Future groups | MLS research |

Rules:

1. Use reviewed protocols/libraries where possible.
2. Do not invent cryptographic primitives.
3. Bind identity verification to Telegram account, TGVerity identity key, device, and protocol version.
4. Treat per-device identity as MVP default.
5. Require external crypto review before strong public security claims.

---

## Localization

All TGVerity strings must use Telegram's localization system.

Requirements:

```text
all Telegram-supported languages
RTL layouts
plural forms where needed
short labels that survive truncation
controlled glossary for security terms
accessible labels for icons and status bars
no hardcoded English UI strings
```

Controlled terms:

| Source term | Must remain distinct from |
|---|---|
| Telegram Cloud | Telegram Secret / TGVerity |
| Telegram Secret | TGVerity Safe |
| TGVerity Encrypted — Unverified | TGVerity Safe |
| TGVerity Safe Relay | TGVerity Safe Direct |
| Key changed — reverify | normal reconnecting |

---

## MVP

### v0.1 Technical Goal

Before product features, TGVerity must prove that the project can build against chosen Telegram sources/libs.

v0.1 should test:

1. Building a minimal Telegram-compatible base.
2. Adding the smallest TGVerity integration point.
3. Sending/receiving text relay packets in a controlled chat.
4. Running early P2P and relay protocol experiments.
5. Verifying that normal Telegram behavior still works.

### Product MVP

Android MVP should be small:

| Feature | MVP |
|---|---:|
| Normal Telegram compatibility | yes |
| TGVerity peer detection | yes |
| 1:1 Telegram relay over text messages | yes |
| QR verification | yes |
| Safety number | yes |
| Key-change warning | yes |
| Safe/Unverified UI labels | yes |
| Direct P2P | v0.1 protocol experiment; product later |
| Groups | no |
| Multi-device identity | no |
| Calls | no |
| iOS/Desktop | later |

Success criteria:

```text
Normal Telegram still works.
Two TGVerity users can detect each other.
They can verify identity via QR/safety number.
They can exchange encrypted TGVerity relay messages.
Telegram sees only opaque TGVerity payloads, not plaintext.
Unverified sessions are never labeled Safe.
Unsafe upstream changes block release.
```

---

## macOS v0.1 Build

Current v0.1 Mac track has two parallel parts:

1. **TGVerity CLI harness** — buildable now; tests text relay packet framing and P2P framing.
2. **Upstream Telegram Desktop build** — used as a clean one-to-one build proof before any surgical UI integration.

CLI build:

```text
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

TDLib-enabled CLI build:

```text
brew install tdlib
cmake -S . -B build-tdlib -G Ninja -DTGVERITY_USE_TDLIB=ON -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build-tdlib
ctest --test-dir build-tdlib --output-on-failure
```

CLI smoke:

```text
build/tgverity relay-pack "hello relay"
build/tgverity p2p-frame "hello p2p"
build-tdlib/tgverity tdlib-version
```

Telegram CLI flow:

```text
export TGVERITY_API_ID=<id>
export TGVERITY_API_HASH=<hash>
export TGVERITY_PHONE=<phone>
build-tdlib/tgverity login
build-tdlib/tgverity chats
build-tdlib/tgverity send-normal <chat_id> "normal hello"
build-tdlib/tgverity send-relay <chat_id> "relay hello"
build-tdlib/tgverity watch [chat_id]
```

P2P smoke:

```text
build-tdlib/tgverity p2p-listen 7777
build-tdlib/tgverity p2p-connect 127.0.0.1:7777 "p2p hello"
```

Telegram Desktop upstream build requires full Xcode, not only Command Line Tools:

```text
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
git clone --recursive https://github.com/telegramdesktop/tdesktop.git
cd tdesktop/Telegram
./build/prepare/mac.sh
./configure.sh -D TDESKTOP_API_ID=<id> -D TDESKTOP_API_HASH=<hash>
open ../out/Telegram.xcodeproj
```

---

## Isolation Boundary

Telegram-side code must access TGVerity only through a narrow bridge.

```text
Telegram UI/message layer
        ↓
tgverity-bridge public API
        ↓
tgverity-core private state
```

Forbidden from Telegram-side code:

```text
TGVerity private keys
session keys / ratchet state
raw decrypted TGVerity plaintext
TGVerity storage files
QR secrets
P2P private metadata
packet internals except approved relay envelope
logs/telemetry containing TGVerity private state
```

---

## Release Safety

TGVerity should track upstream Telegram releases automatically.

A release must be blocked if upstream changes:

```text
break build/tests
reference TGVerity internals
add logging/telemetry near secure paths
change permissions or storage access in risky ways
affect QR/session UI
change network behavior unexpectedly
```

Failures should be reported directly to the owner through at least two channels, e.g. private Telegram bot, email, CI dashboard, issue tracker, or critical push/SMS.

---

## Non-Goals

| Non-goal | Reason |
|---|---|
| Replacing Telegram servers | Normal Telegram accounts still depend on Telegram. |
| Claiming always-serverless communication | P2P can fail; relay may be used. |
| Hidden Telegram transport | No reliable public guarantee. |
| Perfect metadata privacy in relay mode | Telegram still sees transport metadata. |
| Custom cryptography | Too risky. |
| Silent fallback to normal chat | Breaks trust. |
| Group support in MVP | Separate crypto/UX complexity. |
| All platforms from day one | Maintenance burden. |

---

## Open Questions

1. What exactly should official Telegram clients show when they receive TGVerity relay packets?
3. Should unverified encrypted sends be blocked, warned, or allowed with strong labeling?
4. How should mixed Telegram/TGVerity history be separated visually?
5. How should replies, forwards, edits, deletes, search, and notifications work for TGVerity messages?
6. How should media/files handle thumbnails, filenames, previews, and transport names?
7. Should Relay be per-chat opt-in, global opt-in, or prompted after each Direct failure?
8. Which exact protocol stack and library should be selected after crypto review?
9. How should multi-device identity and lost-device recovery work later?
10. How much Telegram UI patching is acceptable before fork maintenance becomes too expensive?
