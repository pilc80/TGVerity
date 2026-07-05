# TGVerity Security Benchmark — 2026-07-05 20:19 MSK

## Scope

Compare TGVerity against **niche security-first messengers**, not mass-market messengers. The goal is not feature parity; the goal is to harden TGVerity's architecture and claims around server listeners, MITM, metadata, and trust UX.

Mass-market apps may remain protocol references, but they are not positioning benchmarks.

## Benchmark set

| Messenger | Core idea | Relevance to TGVerity |
|---|---|---|
| **SimpleX Chat** | no global user IDs; pairwise queues | strongest lesson for identifier minimization |
| **Briar** | P2P/Tor/local mesh; activist model | direct/offline-first transport lessons |
| **Cwtch** | Tor-based metadata-resistant messaging | anti-server-listener architecture lessons |
| **Session** | no phone number; onion-routed service-node network | relay-without-central-account lessons |
| **Ricochet Refresh** | Tor onion identity; no account server | identity-as-address, no directory dependency |
| **Jami** | distributed identity + P2P | direct transport and multi-device lessons |
| **Threema** | paid privacy messenger, QR verification, optional phone/email | practical trust UX and commercial privacy positioning |
| **Status** | Waku/decentralized transport, crypto-native identity | decentralized relay and mobile UX tradeoffs |
| **Tox/qTox** | P2P DHT messenger | cautionary lesson: strong decentralization can hurt UX/reliability |

## Main comparison

| Axis | Best niche pattern | TGVerity target |
|---|---|---|
| **Trust root** | independent app identity, not phone/account | TGVerity identity key; Telegram account is only routing/address context |
| **MITM defense** | QR/safety-number verification; loud key-change warnings | no `Safe` before verified transcript; key change blocks Safe |
| **Server listener resistance** | no central plaintext-capable server | encrypt before Telegram Relay; no TGVerity plaintext server |
| **Metadata resistance** | Tor/onion routing, pairwise queues, no global IDs | impossible in Telegram Relay; only Direct/future relay can reduce metadata |
| **Transport resilience** | multiple paths: P2P, Tor, store-forward | Telegram Relay MVP + Direct experiment; future independent relay optional |
| **Identity discovery** | explicit links/QR/invites; no broad scanning | explicit TGVerity invite; no silent contact probing |
| **Multi-device** | explicit device keys and verification | per-device MVP; multi-device later with visible device list |
| **Groups** | complex; often delayed or MLS-based | non-goal for MVP; MLS research later |
| **Backups** | local/export or encrypted backup only | no cloud plaintext; document recovery before shipping |
| **Security claims** | precise, audited, threat-model-bound | claim only content protection + MITM detection after real crypto/review |

## Messenger-specific lessons

| Source | Copy | Avoid |
|---|---|---|
| **SimpleX** | minimize stable identifiers; prefer pairwise session identity | overpromising anonymity while using Telegram accounts |
| **Briar** | direct/offline-first mindset; threat-model clarity | making Direct mandatory if reliability suffers |
| **Cwtch** | metadata-resistance vocabulary and honesty | implying Telegram Relay has Tor-like metadata protection |
| **Session** | no-phone-number identity idea; untrusted relay concept | complex service-node economics before MVP |
| **Ricochet Refresh** | identity key/address simplicity | desktop-only UX ceiling |
| **Jami** | P2P fallback ideas; distributed account lessons | unreliable NAT traversal as default promise |
| **Threema** | QR verification UX; paid privacy trust language | relying on jurisdiction/brand instead of technical guarantees |
| **Status** | decentralized messaging roadmap | crypto-wallet/product sprawl |
| **Tox** | pure P2P ambition | weak mobile reliability and fragmented UX |

## TGVerity differentiator

```text
Niche secure messengers ask users to move networks.
TGVerity lets users keep Telegram compatibility while adding an independently verified secure layer.
```

This is also TGVerity's constraint: Telegram Relay cannot provide the same metadata resistance as Tor/P2P/pairwise-queue systems.

## Claim matrix

| Claim | Allowed? | Required qualifier |
|---|---:|---|
| “Telegram-compatible secure overlay” | ✅ | after real E2EE exists |
| “Telegram cannot read TGVerity message content” | ✅ | Relay metadata still visible; audited crypto required |
| “MITM-detectable secure sessions” | ✅ | only after QR/safety-number verification |
| “No silent downgrade to normal Telegram” | ✅ | state machine + UI invariant |
| “Anonymous messenger” | ❌ | Telegram account metadata exists |
| “Metadata-private relay” | ❌ | false for Telegram Relay |
| “Serverless messenger” | ❌ | Relay uses Telegram; Direct is best-effort |
| “Government-proof” | ❌ | impossible/overbroad |
| “More secure than Signal/SimpleX/Briar” | ❌ | wrong frame; different constraints |

## Architecture decisions produced by benchmark

| Decision | Rationale |
|---|---|
| **Independent TGVerity identity key** | secure niche tools do not use transport account as trust root |
| **Verification transcript binds Telegram account + TGVerity key + device** | Telegram account takeover or key substitution must be visible |
| **Per-device identity MVP** | avoids hidden multi-device key sharing; matches explicit trust UX |
| **Relay and Direct are separate trust labels** | content privacy vs transport metadata are different promises |
| **No central TGVerity relay in MVP** | avoid creating a second coercible server before need is proven |
| **Future independent relay must be metadata-designed from day one** | if added, use pairwise queues/onion routing lessons, not a normal server |
| **Groups deferred** | mature secure group UX/protocol is hard; MLS or equivalent required |
| **Protocol docs public before claims** | security-first audience needs inspectable design |
| **External review before public security release** | credibility gate, not optional polish |

## Product implications

| Product area | Direction |
|---|---|
| **Target user** | people who need Telegram reach but distrust Telegram-readable cloud content |
| **Primary promise** | independently verified content confidentiality inside Telegram-compatible UX |
| **Primary warning** | Relay does not hide relationship/timing/size metadata from Telegram |
| **Upgrade path** | verified Relay first, verified Direct second, independent metadata-resistant relay later only if needed |
| **UX tone** | precise labels, no fear-marketing, no absolute safety claims |

## Roadmap consequences

```text
1. Threat model and benchmark docs become architecture inputs.
2. Real crypto replaces IdentityCryptoProvider before any public security claim.
3. Verification UX lands before `Safe` labels ship.
4. Desktop/Android platform work must preserve the narrow adapter boundary.
5. Direct/P2P is product-significant because it reduces Telegram transport visibility.
6. Independent relay is future research, not MVP.
```

## Open benchmark questions

1. Should TGVerity eventually offer a **Telegram-free contact path** for verified peers?
2. Should Direct mode use WebRTC, Noise over TCP/QUIC, Tor, or multiple transports?
3. Should identity recovery be **no recovery**, encrypted recovery phrase, or verified device transfer?
4. Should Telegram Relay packet cleanup be automatic, manual, or configurable?
5. Which security claims are allowed before external review, if any beyond “prototype”? 
