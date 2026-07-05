# TGVerity Roadmap — 2026-07-05 20:45 MSK

## Goal

Finish TGVerity as a Telegram-compatible secure overlay that runs predictably across supported platforms without weakening the trust model.

```text
Current reality: C++ core + TDLib CLI + macOS/Desktop prototype track.
Product target: verified Relay MVP, then Direct, then broader platforms.
```

## North-star invariants

1. Normal Telegram behavior remains normal.
2. `Safe` is shown only after TGVerity identity verification.
3. Relay protects content, not Telegram metadata.
4. Direct is best-effort and separately labeled.
5. Telegram-side code never receives private keys, session state, or plaintext.
6. Public security claims require real crypto and external review.

## Phase plan

| Phase | Target | Done when |
|---|---|---|
| **P0 Docs alignment** | single architecture package | roadmap, adapter contract, crypto plan, build matrix, testing plan exist and link from README |
| **P1 Core hardening** | portable protocol core | packet/state/store/bridge tests pass; real crypto provider selected; insecure mode blocked for release |
| **P2 Desktop/macOS proof** | runnable patched app | branded TGVerity app builds; normal Telegram works; TGVerity packet send/receive path is wired and smoke-tested |
| **P3 Android MVP** | primary mobile MVP | Android adapter/hook map implemented; QR/safety verification; verified Relay E2E on two accounts/devices |
| **P4 Desktop cross-platform** | Linux/Windows Desktop | same Desktop adapter builds/runs or blockers are documented per OS |
| **P5 iOS feasibility** | go/no-go | storage, notifications, App Store, fork maintenance, and verification UX constraints documented |
| **P6 Future transports** | metadata reduction | Direct transport and/or independent relay evaluated against threat model |

## Platform order

| Order | Platform | Reason |
|---:|---|---|
| 1 | **macOS CLI / TDLib** | already works; fastest regression harness |
| 2 | **Telegram Desktop macOS** | current patch/branding work exists; proves GUI integration |
| 3 | **Android** | product MVP platform; needs real mobile UX/security hooks |
| 4 | **Desktop Linux/Windows** | reuse Desktop work after macOS proof |
| 5 | **iOS** | high maintenance/review risk; feasibility first |
| 6 | **Web/headless** | optional/testing/research path |

## Critical blockers

| Blocker | Blocks |
|---|---|
| Identity/no-op crypto remains active | public security release |
| Desktop hook patch remains DRAFT/unwired | Desktop proof |
| No Android hook map | Android MVP |
| No two-account E2E smoke | claiming Relay works in real Telegram |
| No notification/search leakage tests | claiming Telegram-side plaintext isolation |
| No upstream patch drift check | reliable fork maintenance |

## Release milestones

| Milestone | Scope | Security claim allowed |
|---|---|---|
| **v0.2 prototype** | core + CLI + Desktop shim draft | none; insecure prototype only |
| **v0.3 real crypto alpha** | real AEAD provider + tamper/replay tests | content encrypted in test harness |
| **v0.4 Desktop proof** | patched Desktop relay path | prototype content protection, no audit claim |
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

1. Finalize P0 docs.
2. Add release gate preventing IdentityCryptoProvider in non-prototype builds.
3. Pin Desktop upstream commit and finalize hook patch placement.
4. Run macOS Desktop two-account relay smoke.
5. Draft Android hook map before Android code work.
