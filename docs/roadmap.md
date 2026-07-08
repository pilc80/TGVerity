# TGVerity Roadmap — 2026-07-06 15:16 MSK

## Goal

Finish TGVerity as a Telegram-compatible secure overlay that runs predictably across supported platforms without weakening the trust model.

```text
Current reality: C++ core + TDLib CLI + branded inert Desktop prototype.
Product target: security-gated two Telegram Desktop clients, then Android MVP.
```

## North-star invariants

1. Normal Telegram behavior remains normal.
2. `Safe` is shown only after TGVerity identity verification.
3. Relay protects content, not Telegram metadata.
4. Direct is best-effort and separately labeled.
5. Telegram-side raw message/search/notification/storage paths never receive plaintext.
6. Public security claims require real crypto and external review.

## Phase plan

| Phase | Target | Done when |
|---|---|---|
| **P0 Docs/spec lock** | desktop security architecture package | desktop two-client spec, roadmap, adapter contract, crypto plan, build matrix, testing plan agree |
| **P1 Core security** | portable secure-mode core | real crypto/session path selected; AEAD/AAD/replay/key-change tests pass; insecure mode blocked for secure builds |
| **P2 Desktop/macOS proof** | two patched Desktop users | branded TGVerity app builds; normal Telegram works; QR verification; TGVerity Relay sends opaque carrier; raw UI/notif/search/log leakage checks pass |
| **P3 Android MVP** | primary mobile MVP | Android adapter/hook map implemented; QR/safety verification; verified Relay E2E on two accounts/devices |
| **P4 Desktop cross-platform** | Linux/Windows Desktop | same Desktop adapter builds/runs or blockers are documented per OS |
| **P5 iOS feasibility** | go/no-go | storage, notifications, App Store, fork maintenance, and verification UX constraints documented |
| **P6 Future transports** | metadata reduction | Direct transport and/or independent relay evaluated against threat model |

## Platform order

| Order | Platform | Reason |
|---:|---|---|
| 1 | **macOS CLI / TDLib** | fastest regression harness for real Telegram transport |
| 2 | **Telegram Desktop macOS** | current priority: two-client security-proof GUI path |
| 3 | **Android** | product MVP platform after Desktop security proof/hook lessons |
| 4 | **Desktop Linux/Windows** | reuse Desktop work after macOS proof |
| 5 | **iOS** | high maintenance/review risk; feasibility first |
| 6 | **Web/headless** | optional/testing/research path |

## Critical blockers

> Updated from code review 2026-07-07 (see `docs/code-review-findings.md` and `docs/architecture-gaps.md`).

| Blocker | Blocks | Finding |
|---|---|---|
| Identity/no-op crypto remains active | public security release | C2 (AAD), C3 (redaction) |
| Desktop hook patch remains DRAFT/unwired | Desktop proof | H1 |
| SessionStore zero persistence | replay protection | C1 |
| AAD binds only chatId, not full context | cryptographic integrity | C2 |
| Logger redaction user-tunable | log security guarantee | C3 |
| ACK not suppressed from raw UI | packet security | C4 |
| Plaintext persists in bridge memory | memory security | H3 |
| No Android hook map | Android MVP | roadmap |
| No two-account E2E smoke | claiming Relay works | roadmap |
| No notification/search leakage tests | plaintext isolation | roadmap |
| No upstream patch drift check | fork maintenance | roadmap |
| No X3DH session protocol design | C3 milestone | architecture-gaps.md §2 |
| No Double Ratchet design | C4 milestone | architecture-gaps.md §3 |
| C ABI bridge unimplemented | Desktop shim drift | architecture-gaps.md §8 |

| Blocker | Blocks |
|---|---|
| Identity/no-op crypto remains active | public security release |
| Desktop hook patch remains DRAFT/unwired | Desktop proof |
| No Android hook map | Android MVP |
| No two-account E2E smoke | claiming Relay works in real Telegram |
| No notification/search leakage tests | claiming Telegram-side plaintext isolation |
| No upstream patch drift check | reliable fork maintenance |
| No X3DH/session protocol design | first encrypted message over relay |
| No Double Ratchet design | forward secrecy / post-compromise recovery |

## Release milestones

| Milestone | Scope | Security claim allowed |
|---|---|---|
| **v0.2 prototype** | core + CLI + Desktop shim draft | none; insecure prototype only |
| **v0.3 secure core alpha** | real AEAD/session/identity + QR transcript tests | content encrypted in test harness, not public security claim |
| **v0.4 Desktop two-client proof** | patched Desktop + QR + verified Relay + leakage gates | internal alpha: Telegram sees opaque carrier under tested threat model |
| **v0.5 Android MVP alpha** | verified Relay E2E | limited alpha claim with clear metadata caveat |
| **v1.0 reviewed MVP** | external review + release gates | threat-model-bound security claim |

## Acceptance criteria for “runs correctly”

```text
Build succeeds.
Normal Telegram flow still works.
TGVerity flow works through the platform adapter.
Trust labels match verification state.
Raw packets/plaintext do not leak through render/search/notification paths.
Tests/smokes are documented and repeatable.
Known non-guarantees are visible in docs.
```

## Next implementation focus

> Updated from code review 2026-07-07 and architecture-gaps 2026-07-07.

1. Fix critical code gaps: ACK suppression (C4), log redaction gating (C3), plaintext wipe (H3), empty-body parser (L4).
2. Add SessionStore persistence API (C1) — disk-backed or at minimum save/load.
3. Expand AAD from `chatId` to full context blob (C2).
4. Design X3DH session protocol (C3) — prekey bundle format, KeyExchangeMessage, session state machine, nonce management spec, HKDF KDF spec.
5. Design Double Ratchet state layout (C4) — root key, chain keys, per-message MAC key derivation, forward secrecy guarantee.
6. Implement C1 AEAD provider (libsodium XChaCha20-Poly1305) replacing IdentityCryptoProvider in secure mode.
7. Wire Desktop C ABI bridge — eliminate shim duplication, fix drift risk.
8. Extend core adapter contract for QR/trust UI, secure storage, all-packet suppression, async cleanup.
9. Implement secure core tests before Desktop Safe UX (tamper, AAD mismatch, replay, restart, release-block).
10. Wire Desktop hook patch (H1) — send/incoming/delete/notif/edit.
11. Use TDLib two-account smoke as the real Telegram transport gate.
12. Draft Android hook map after Desktop proof lessons.
13. Add CI upstream drift detection job (weekly tdesktop dev branch poll).
