# TGVerity Desktop Two-Client Security-Proof Spec — 2026-07-06 15:16 MSK

## Context

Goal: two patched Telegram Desktop users can exchange TGVerity messages through ordinary Telegram cloud chats while Telegram-side protocol, history, search, raw UI, notifications, and logs never receive TGVerity plaintext.

```text
Alice plaintext
  -> tgverity-core encrypts/authenticates
  -> Telegram Desktop sends opaque carrier text
  -> Telegram cloud stores/sees opaque carrier only
  -> Bob Desktop suppresses raw carrier
  -> tgverity-core decrypts
  -> Bob sees TGVerity-owned virtual message
```

This spec targets **current Telegram Desktop fork path** first. Android starts only after this desktop proof or an accepted hook map. “Security proof” means bounded, repeatable evidence against the stated threat model; it is not a public audited security claim.

## Security meaning of “unrevealed”

| Protected from | Required result |
|---|---|
| Telegram cloud/protocol | no TGVerity plaintext, keys, safety numbers, or session state in carrier text |
| Telegram local raw history/search | no TGVerity plaintext indexed/stored as Telegram message text |
| Raw Telegram bubbles | carrier packet hidden on patched TGVerity Desktop clients |
| Raw Telegram notifications | carrier packet never previews/notifies on patched TGVerity Desktop clients |
| Logs/telemetry | no plaintext, packet bodies, private keys, session keys, safety secrets in known TGVerity/Desktop logs |

Non-guarantees: Telegram still sees accounts, timing, delivery behavior, approximate packet size, and encrypted carrier history. Other logged-in official/unsupported Telegram clients may display opaque carrier text. TGVerity does not protect compromised endpoints, screenshots, malicious recipients, OS notification capture after plaintext preview, or future upstream drift unless gates keep passing.

## Required architecture

```text
Telegram Desktop hook sites
        |
        v
Desktop adapter        # ids, send/delete, raw UI/notif suppression, virtual render, QR UI
        |
        v
tgverity-core          # identity, verification transcript, sessions, AEAD/ratchet, replay, state
        |
        v
platform secure store  # private identity/session/replay/trust state
```

Boundary rules:

1. Telegram send APIs receive only opaque carrier text for TGVerity messages.
2. Telegram-side raw message/search/history/notification code never receives decrypted plaintext.
3. Desktop hooks call adapter methods only; crypto/session logic stays in `tgverity-core`.
4. `Safe` is impossible before verified identity transcript.
5. Cleanup/deletion is best-effort hygiene, never a security guarantee.

## Desktop hook contract

| Hook | Desktop purpose | Must call / enforce |
|---|---|---|
| Send wrap | intercept TGVerity-mode outgoing plaintext before Telegram send | core send; carrier text only; disable previews/entities |
| ID bind | map local/random id to server msg id | `onMessageIdBound` for cleanup/state |
| Incoming text | detect candidate carrier text | authenticate/decrypt via core; prefix alone never trusted |
| Raw render suppress | hide raw carrier bubble | all TGVerity packet types, including ACK/malformed candidates policy |
| Virtual render | show decrypted message | TGVerity-owned bubble/model, not Telegram message text mutation |
| Notification suppress | prevent raw carrier notification | suppress before notification emission race |
| Virtual notification | show controlled TGVerity notification | preview only from TGVerity-owned decrypted state and user setting |
| Edit suppress | raw/virtual TGVerity messages immutable for MVP | corrections are new TGVerity messages |
| Delete revoke | cleanup raw MSG/ACK | one revoke/delete-for-all call, batch <=100, never self-delete first |
| QR/trust UI | in-chat verification | QR display/scan/import, verified/unverified/key-changed states |
| Secure storage | persistent secrets/state | identity private key, session state, replay cache, verification state |

## In-chat QR verification UX

Minimum user flow:

```text
Open chat
  -> header shows TGVerity Available / Encrypted — Unverified
  -> user opens TGVerity panel
  -> Alice shows QR; Bob scans/imports QR
  -> clients exchange an authenticated confirmation packet for the same transcript hash
  -> both clients show TGVerity Safe Relay only after mutual confirmation
```

QR/safety transcript must bind:

```text
Telegram account ids
chat/peer id
both TGVerity identity public keys
both device ids
protocol version
crypto/session suite
session challenge/expiry
negotiated parameters hash
```

Neither side may show `Safe` until it has local transcript validation plus peer confirmation for the same transcript hash. Manual safety-number comparison can replace QR scan only if both users explicitly confirm the same value.

UX states:

| State | Send behavior | Label |
|---|---|---|
| Normal Telegram | normal Telegram send | Telegram Cloud |
| TGVerity available | user can start setup | TGVerity Available |
| Encrypted but unverified | allow only if policy explicitly permits; never Safe | TGVerity Encrypted — Unverified |
| Verified relay | TGVerity send path default for that chat | TGVerity Safe Relay |
| Key changed | block Safe; require reverify | Key changed — reverify |
| Error/leakage risk | block TGVerity send | TGVerity unavailable |

## Crypto/session requirements

Security-gated two-client testing is blocked until `IdentityCryptoProvider` is replaced for secure mode.

Required minimum for desktop proof:

1. Reviewed library/protocol choice, not custom primitives.
2. Authenticated encryption with AAD binding chat/account/device/session/packet context.
3. Persistent per-device identity key.
4. QR transcript verification before `Safe`.
5. Replay/counter cache durable across restart.
6. Key-change detection that removes `Safe` and blocks silent accept.
7. Release/secure build fails if identity/no-op crypto is active.
8. Logs hard-redact by build policy, not user preference only.

Recommended implementation direction:

```text
Alpha desktop proof: libsodium-backed AEAD + explicit verified identity/session transcript; no forward-secrecy/PCR claim unless a ratchet is implemented
Reviewed MVP: X3DH-style async setup + Double Ratchet-style messaging, or vetted equivalent
```

## Message lifecycle

```text
Send TGVerity message
  -> core checks trust/chat mode
  -> core encrypts packet with bound AAD
  -> adapter sends opaque carrier with previews/entities disabled
  -> ID bind marks raw MSG server id
  -> local virtual message shows pending/sent

Receive TGVerity carrier
  -> raw render/notif/edit suppressed before user-visible surfacing
  -> core authenticates, replay-checks, decrypts
  -> virtual message stored/rendered by TGVerity path
  -> ACK carrier sent
  -> raw MSG/ACK cleanup queued/retried best-effort
```

Failure policy:

| Failure | Result |
|---|---|
| malformed carrier | no plaintext; raw handling per unsupported/suspicious policy; no trust upgrade |
| auth/AAD failure | no plaintext; optional warning; no ACK as valid message |
| replay | ignore/flag; no duplicate virtual message |
| cleanup failure | keep virtual message state; retry or record cleanup pending |
| notification race risk | block TGVerity readiness until fixed |
| key changed | stop Safe, require QR/safety reverify |

## Implementation phases

| Phase | Target | Done when |
|---|---|---|
| D0 Spec lock | docs/contracts aligned | this spec, roadmap, crypto plan, adapter contract, tests agree |
| D1 Core security | real secure-mode core | AEAD/session/identity/replay/key-change tests pass; insecure secure-build blocked |
| D2 Adapter contract | Desktop-ready API spec + code migration plan | contract distinguishes implemented hooks from planned QR/trust/store/scanner hooks |
| D3 TDLib protocol gate | implementable real Telegram transport harness | concrete TDLib adapter + two accounts exchange opaque TGVerity packets with bounded history/search/log scans |
| D4 Desktop hook patch | wired app | send/incoming/id/render/notif/edit/delete hooks present and patch applies to pinned tdesktop |
| D5 Desktop UX | native proof | in-chat QR, trust label, virtual bubble, controlled notifications work |
| D6 Two-client smoke | milestone evidence | two Desktop users pass normal chat + verified TGVerity Relay + leakage checklist |

## Acceptance checklist for two Desktop users

1. Two separate TGVerity Desktop clients/profiles/accounts launch without touching Telegram.app data.
2. Normal Telegram send/receive works unchanged.
3. In-chat QR verification transitions both clients to `TGVerity Safe Relay`.
4. Alice sends sentinel plaintext through TGVerity mode.
5. Telegram carrier text is opaque and never contains sentinel plaintext.
6. Bob sees only TGVerity virtual message, not raw carrier bubble.
7. Bob receives no raw carrier notification; virtual notification is controlled.
8. Edit affordance is hidden for raw/virtual TGVerity messages.
9. ACK/state reaches `tgverity_ack`; raw MSG/ACK cleanup is queued/invoked and result recorded.
10. Known accessible Telegram search/history/raw surfaces do not contain sentinel plaintext.
11. TGVerity/Telegram logs do not contain sentinel plaintext, packet body, keys, safety numbers, or session secrets.
12. Key-change simulation removes `Safe` and requires reverify.
13. All evidence records TGVerity commit, tdesktop commit, OS, accounts type, commands, screenshots/log scan result, and known failures.

## Release blockers

> Updated from code review 2026-07-07 and architecture-gaps 2026-07-07. See `docs/code-review-findings.md` and `docs/architecture-gaps.md`.

```text
Virtual message persistence model is undefined
QR verification lacks mutual transcript confirmation
secure mode lacks selected key agreement/session/KDF design
unsupported/other-client carrier visibility is not documented
Identity/no-op crypto active in secure mode
Safe visible before verified transcript
plaintext reaches Telegram send/search/history/raw notification/log path
raw TGVerity carrier can notify or render in TGVerity-capable chat
Desktop hook patch is absent, inert, or drifted from pinned upstream
ACK packets are not suppressed from raw UI/notifs
key change can be silently accepted
cleanup is described as guaranteed deletion
README/docs claim more than tests prove
replay cache / session counter lost on restart (no persistence)
plaintext persists in bridge memory after cleanup_done
AEAD AAD binds only chatId, not full cryptographic context
core parser accepts empty body for relay_text (shim rejects)
X3DH session protocol unspecified — no first encrypted message possible
Double Ratchet unspecified — zero forward secrecy
C ABI bridge unimplemented — shim codec drifts from core nonce management spec not designed — first AEAD provider unsafe
per-message nonce management not designed — AEAD nonce reuse = catastrophic key compromise
memory zeroization policy not defined — plaintext leaks on crash/dump
release build does not fail when insecure provider is active
adapter lacks secure storage implementation (K hook)
upstream drift detection not automated — patch may silently break on next tdesktop release
```

## Critical files

| Area | Files |
|---|---|
| Core bridge/adapter | `src/tgverity/bridge.*`, `src/tgverity/tg_client.h` |
| Crypto/session | `src/tgverity/crypto_provider.*`, `src/tgverity/session_store.*` |
| Fake/test adapter | `src/tgverity/fake_telegram_client.*`, `tests/bridge_tests.cpp` |
| TDLib harness/adapter | `src/tg/td_client.*`, `src/tgverity/tg_client_tdlib.cpp` |
| Desktop shim/patch | `desktop/tgverity_bridge_shim.*`, `patches/tdesktop-tgverity.patch` |
| Docs/contracts | `docs/crypto-plan.md`, `docs/platform-adapter-contract.md`, `docs/message-lifecycle.md`, `docs/testing-plan.md` |

## Known code review findings

See `docs/code-review-findings.md` for the full audit of this codebase.
Key gaps to resolve before D6 smoke:

```text
C1  SessionStore: zero persistence → replay cache lost on restart
C2  AAD under-binding: only chatId, not full context
C3  Logger redaction: user-tunable, not build-enforced
C4  ACK packets: not suppressed from raw UI/notifs
H3  Plaintext never cleared from bridge memory
L4  Core parser: accepts empty relay_text body (shim rejects)
```

## Open design decisions before code

1. Exact crypto library/protocol for alpha secure mode.
2. Whether unverified encrypted sends are blocked or allowed with strong warning; `Safe` remains blocked either way.
3. Desktop virtual message storage model: memory-only, TGVerity encrypted local DB, or hybrid.
4. QR scanner implementation on Desktop: camera, screenshot/import, or manual code first.
5. Oversized message policy: split encrypted packets vs encrypted file carrier.
6. Multi-device identity is out of first proof unless explicitly added.
