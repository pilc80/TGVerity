# TGVerity Architecture Gap Analysis — 2026-07-07

Compares project docs + code against community best practices in each knowledge area. Gaps are **observed** (docs or code confirm them).

---

## 1. Cryptography — Gap: ⚠️ HIGH

| Area | Best practice | TGVerity status | Gap severity |
|---|---|---|---|
| **Algorithm** | libsodium AEAD (XChaCha20-Poly1305 or AES-GCM) | `IdentityCryptoProvider` — plaintext, zero encryption | 🔴 Critical. Blocks all security claims. |
| **Key derivation** | HKDF per RFC 5869 with distinct `info` context | None defined yet | 🟡 Missing design. C2 plan names it but no spec. |
| **Nonce management** | Per-message unique nonce; counter-based; never repeat | Not designed (placeholder crypto) | 🔴 #1 AES-GCM failure mode across all custom protocols. Must be specified before C1. |
| **AAD binding** | Identity keys + session ID + protocol version + peer IDs + packet counter | C2 plan lists 11 fields; code binds only `chatId` | 🔴 Partial. C2 spec exists; C1 code doesn't implement it. |
| **Key storage** | OS secure enclave (macOS Keychain, Android Keystore, iOS Keychain) | No storage abstraction beyond `SessionStore` in-memory | 🟡 Platform adapter contract names `K secure storage` as required but no implementation exists. |
| **Crypto agility** | Enum of cipher suites; runtime selection; migration path | Enum exists (`CryptoSuite`); only Identity variant | 🟢 Good structure. Ready for C1 libsodium plug-in. |
| **Compile-time guard** | Build fails if insecure provider linked in release | Warns once per session; no build-block | 🔴 Code-review finding C3 (log redaction) overlaps; same root: no build policy enforcement. |
| **Memory sanitization** | `sodium_memzero()` post-use; `secure_alloc` | None | 🟡 libsodium pattern not yet adopted. |
| **Post-quantum** | Signal uses PQXDH (X25519 + ML-KEM-768 hybrid) since 2023 | Not considered | 🟢 Low priority for MVP; note in roadmap. |

**Verdict:** C0 (prototype seam) is structurally sound. The interface design enables C1. The gap is **not architecturally** — it's **schedule**: C1 AEAD provider is the single biggest blocker on the critical path.

---

## 2. Key Agreement & Session Establishment — Gap: ⚠️ HIGH

| Area | Best practice (Signal X3DH) | TGVerity status | Gap severity |
|---|---|---|---|
| **Identity key pair** | X25519; per-device; persistent | "Persistent per-device identity key" required in spec; no implementation | 🟡 C2 milestone not started |
| **Prekey bundles** | Server-mediated; batch upload (~100); one-time keys | Not designed; relay is just text carrier | 🟡 No key directory protocol defined |
| **X3DH handshake** | 4 DH computations → HKDF master secret → AEAD session key | Not implemented | 🔴 C3 milestone not started |
| **One-time prekey (1:K)** | Provides post-compromise recovery; server queue | Not designed | 🟡 Without 1:K, no PCR on initial session |
| **Session protocol (Signal)** | Named sessions, keyed sessions, indexed keys — state machine for key management | Not designed | 🔴 libsignal has a full session state machine; prototype skips it entirely |

**Gap rationale:** The crypto plan says "X3DH-style async setup + Double Ratchet" at C2-C4, but no protocol spec exists. Signal's session layer spec (signal.org/docs/specifications/sessions/) defines named sessions, keyed sessions, and indexed prekeys — a substantial state machine. Without it, session re-establishment and prekey sync are undefined.

**Critical missing design:** Prekey bundle validation — Signal requires signature verification of Bob's signed prekey AND cross-key-binding (all keys in bundle belong to same identity). Prototypes that skip this allow MITM key substitution in the bundle itself.

---

## 3. Message Ratcheting — Gap: ⚠️ HIGH

| Area | Best practice (Signal Double Ratchet) | TGVerity status | Gap severity |
|---|---|---|---|
| **Symmetric ratchet** | HKDF chain key advances after every message; per-message key derived | Not designed | 🔴 C4 not started |
| **Asymmetric ratchet** | New X25519 ephemeral key per ratchet step; post-compromise recovery | Not designed | 🔴 C4 not started |
| **Forward secrecy** | Compromising session key does NOT compromise past messages | Impossible without ratchet | 🔴 Current design offers zero forward secrecy |
| **Post-compromise recovery** | Fresh DH entropy on each remote ratchet step | Not designed | 🔴 Same as above |
| **Message number overflow** | Counter rotation at ~2^53; Signal uses 64-bit chain counters | Not designed | 🟡 Low risk for MVP volume but must be in spec |

**Gap rationale:** Without a ratchet, the protocol provides only **static encryption** (same key for all messages in a session). This means: one key compromise → all past AND future messages compromised. Signal's ratchet is what gives E2EE messengers their security guarantees. The crypto plan correctly identifies C4 as required but no implementation path exists.

---

## 4. Identity Verification — Gap: ⚠️ MEDIUM

| Area | Best practice (Signal/Session/Threema) | TGVerity status | Gap severity |
|---|---|---|---|
| **QR data** | Encrypted payload with public keys, fingerprint, session URL | Spec defines transcript fields; QR wire format undefined | 🟡 C2 needs QR envelope spec |
| **Mutual confirmation** | Both sides must exchange auth confirmation before `Safe` | Spec requires it; protocol not designed | 🟡 Gap between spec intent and protocol design |
| **Safety number comparison** | 60-bit fingerprint (Signal: 16 decimal digits, SHA256 short hash) | Mentioned as fallback; no algorithm defined | 🟡 Signal uses `SHA-256 truncated to 60 bits`; base-16 digits |
| **Key-change detection** | Persistent identity key; compare on reconnect; loud warning | State machine has `tgverity_key_changed`; no detection trigger defined | 🟡 State exists; trigger mechanism missing |
| **Recovery** | Encrypted recovery phrase / verified device transfer (Session) | "No recovery" or "later" — no decision | 🟡 Explicit gap. Per benchmark: "Should identity recovery be no recovery, encrypted recovery phrase, or verified device transfer?" |
| **Cross-device** | Multi-device: primary phone + up to 5 linked devices (Signal) | Explicitly deferred to post-MVP | 🟢 Correctly scoped out |

**Gap rationale:** The transcript fields are well-specified in the desktop security spec. The gap is the **QR envelope wire format** (what binary layout goes inside the QR code), the **key-change detection trigger** (what event fires the comparison), and the **recovery strategy** (still an open benchmark question).

---

## 5. P2P / Direct Transport — Gap: ⚠️ MEDIUM

| Area | Best practice (Signal SIMP / Briar / WebRTC) | TGVerity status | Gap severity |
|---|---|---|---|
| **NAT traversal** | ICE/STUN/TURN with candidate prioritization | `p2p_socket` exists as raw TCP; no STUN/TURN/ICE | 🔴 Two mobile peers behind CGNAT cannot be hole-punched (~29% of networks) |
| **Protocol** | QUIC (0-RTT, connection migration) or WebRTC ICE | Raw TCP socket | 🟡 MVP accepts this but production P2P needs QUIC/WebRTC |
| **Fallback** | Encrypted relay (Signal-style) when P2P fails | "ask: Relay or Cancel" — UX flow exists, relay encryption not specified for P2P path | 🟡 Fallback behavior is correct; P2P relay encryption undefined |
| **Battery** | TCP keepalive, ICE streaming (non-blocking), NAT mapping renewal | Raw TCP with no keepalive | 🟡 Mobile-unfriendly but acceptable for experimental |
| **Metadata** | Signal SIMP: delayed delivery, no read receipts, no online status | Not addressed for Direct | 🟡 Direct mode doesn't reduce metadata vs relay unless designed to |

**Gap rationale:** The project correctly labels Direct as "experimental" for v0.1. The gap is that `p2p_socket.cpp` implements raw TCP with no NAT traversal. Even experimental P2P needs at minimum STUN for discovery to work across home routers. **Not a blocker** — the spec acknowledges this is later work.

---

## 6. Metadata Resistance — Gap: ⚠️ MEDIUM

| Area | Best practice (SimpleX / Cwtch / Session) | TGVerity status | Gap severity |
|---|---|---|---|
| **Transport metadata** | Tor hidden services; pairwise queues; no central server | Telegram cloud carries packets — full metadata visible | 🟡 Correctly documented as non-guarantee; no attempt to hide |
| **Traffic analysis** | Cover traffic; padding; dummy messages | Not implemented | 🟡 Out of scope for MVP; documented |
| **Contact discovery** | Session URL / QR / link — no phone number required | Requires Telegram account as discovery | 🟡 Correct trade-off for MVP |
| **Read receipts / online status** | Disabled by design (Signal SIMP) | Not specified for Telegram relay | 🟡 Relay inherits Telegram's default; not a TGVerity problem |

**Gap rationale:** TGVerity's docs correctly state metadata resistance is impossible in Telegram Relay mode. The benchmark doc acknowledges this. **No architectural gap** — the threat model is honest. The only gap is that Direct mode's metadata reduction benefits are not designed (raw P2P reveals IP to peer, which may or may not be acceptable).

---

## 7. Plaintext Isolation — Gap: ⚠️ MEDIUM

| Area | Best practice (Signal Android / Briar) | TGVerity status | Gap severity |
|---|---|---|---|
| **Send path** | Opaque carrier text only; link previews disabled; no entities | Bridge sends encrypted packet; preview/entity control delegated to adapter | 🟡 Correct separation of concerns (core ≠ platform) |
| **Receive path** | Suppress raw render + raw notification; render virtual TGVerity message | `bridge.cpp` calls `suppressRawRender/Notification` on adapter | 🟡 Contract correctly separates responsibilities |
| **ACK suppression** | All TGVerity packet types suppressed from raw UI | ACK suppression mentioned but C4 finding says "ACK not suppressed" | 🔴 Code review finding C4 — ACK packets may render as raw UI |
| **Plaintext wipe** | `sodium_memzero()` on decrypted buffers | Bridge test shows plaintext never cleared from memory | 🔴 Code review finding H3 |
| **Notification paths** | FCM/APNs data payload encrypted; client decrypts before rendering | Spec defines virtual notification; Android/iOS notification extension not designed | 🟡 Android notification suppression is a P3 gap (no Android hook map yet) |
| **Log redaction** | Build-enforced, not user-tunable | Logger is user-tunable with `setVerbosity()` | 🔴 Code review finding C3 — user can disable redaction at runtime |
| **Search/index** | Virtual messages not indexed by system search | No mechanism defined | 🟡 Desktop Telegram search index is a known unknown |

**Gap rationale:** The core architecture (adapter pattern) is correct. The gaps are implementation-level: memory cleanup, ACK suppression, and log redaction enforcement. These are **fixable code bugs**, not design flaws. The pattern is right.

---

## 8. Desktop Patching / Integration — Gap: ⚠️ HIGH

| Area | Best practice (libsignal pattern) | TGVerity status | Gap severity |
|---|---|---|---|
| **Integration approach** | C ABI bridge (libsignal's `extern "C"` + opaque handles) | `tgverity_bridge_shim.h` — standalone C++ duplication, not C ABI | 🔴 Shim duplicates codec logic from core. C ABI plan exists (desktop/README.md) but unimplemented. |
| **Upstream tracking** | CI drift detection + pinned commit | No automated drift detection; patch documented as "DRAFT/unwired" | 🟡 H1 finding — Desktop proof blocked until H1 wired |
| **Hook surface** | Minimal: send-wrap, incoming-text, delete-revoke, notification suppress | 10 hooks specified; TDLib harness partially wired; real Desktop hooks absent | 🟡 Contract well-defined; implementation deferred to D3-D4 |
| **Version pinning** | Pin to specific commit; weekly upstream poll | Not implemented | 🟡 Mentioned as gap in roadmap; no CI job for it |
| **Secure storage** | Keychain/Keystore abstraction via adapter | `secureStoreGet/Put/Delete` in contract; no implementation | 🟡 K hook required before real crypto release |
| **tde2e coexistence** | Telegram's own tde2e module exists — isolate from it | No awareness documented | 🟡 tgverity should treat tde2e as separate namespace; no conflict expected (different key space) |

**Gap rationale:** The C ABI bridge plan is documented but not implemented. The shim is a stopgap that duplicates wire-format code. This is the **maintenance cost** vector — a stable C ABI from core prevents shim drift.

---

## 9. Testing & Verification — Gap: ⚠️ MEDIUM

| Area | Best practice | TGVerity status | Gap severity |
|---|---|---|---|
| **Tamper test** | Bit-flip ciphertext → reject | IdentityCryptoProvider returns plaintext — cannot test AEAD rejection | 🔴 Impossible with current crypto |
| **AAD mismatch** | Copy packet to different chat → reject | Only binds `chatId`; not full context | 🔴 C2 finding |
| **Replay test** | Old packet rejected | `SessionStore` in-memory only; lost on restart | 🔴 C1 finding |
| **Key-change test** | Swap identity key → `Safe` removed | State machine has `tgverity_key_changed`; no trigger | 🟡 Partial |
| **Insecure-build test** | Build fails if IdentityCryptoProvider active | Build succeeds; only logs warning | 🔴 C3 finding |
| **Leakage tests** | Sentinel word scan across all surfaces | Specified in adapter contract (#12); no test harness | 🟡 Planned but unimplemented |
| **E2E two-account smoke** | Real Telegram accounts exchange packets | TDLib harness exists; not yet exercised | 🟡 P3 blocker on roadmap |

**Gap rationale:** Testing plan doc exists. The gaps are that the tests cannot run against real crypto (none exists yet), and the leakage test harness is unimplemented. **Fixable** once D1 core security lands.

---

## 10. Protocol Design — Gap: ⚠️ HIGH

| Area | Best practice | TGVerity status | Gap severity |
|---|---|---|---|
| **Protocol spec** | Full wire format: handshake, session, message, ACK, cleanup | Relay packet format documented (relay_packet.h); session setup NOT specified | 🔴 Without session establishment protocol, first message is undefined |
| **Packet format** | Versioned envelope; type discriminator; AAD fields | RelayPacket struct well-structured (v1 envelope). ACK format defined. | 🟢 Good structure, extendable |
| **Wire versioning** | `v1:` prefix + forward-compatible parser | Implemented and tested | 🟢 Good |
| **Security review** | Formal analysis (ProVerif/Tamarin) + human audit | "External review before public security release" — scheduled at C6 | 🟡 Correctly deferred |
| **Crypto library review** | Use audited, peer-reviewed library | libsodium identified as target; not yet integrated | 🟡 Plan exists; implementation deferred |

---

## Summary: Gap Heatmap

| Knowledge Area | Gap Severity | Priority to Fix |
|---|---|---|
| Crypto (AEAD/provider) | 🔴 Critical | P1 — blocks all security claims |
| Key Agreement (X3DH) | 🔴 High | P1 → P2 (C2 milestone) |
| Ratcheting (Double Ratchet) | 🔴 High | P1 → P2 (C4 milestone) |
| Key Storage (platform) | 🟡 Medium | P1 → P2 (before real crypto release) |
| Desktop C ABI bridge | 🔴 High | P2 (wires shim to core) |
| Plaintext wipe + ACK suppress | 🔴 High | P2 (fix existing code bugs) |
| Identity Verification (QR wire format) | 🟡 Medium | P2 (C2 milestone) |
| P2P/STUN/NAT | 🟡 Medium | P6 (experimental, not MVP) |
| Metadata Resistance | 🟢 Low | Correctly scoped as non-goal |
| Testing harness (leakage/E2E) | 🟡 Medium | P3 (after core security) |
| Upstream drift detection | 🟡 Medium | P2 (CI job) |
| Recovery strategy | 🟡 Medium | Pre-MVP design decision needed |

## Top 5 Action Items

1. **Implement C1 AEAD provider** (libsodium XChaCha20-Poly1305) — replaces `IdentityCryptoProvider`; enables all security tests
2. **Add SessionStore disk persistence** — replay cache survives restart
3. **Wire Desktop C ABI** — replace shim duplication with stable `extern "C"` bridge; blocks all Desktop hook work
4. **Fix memory/log bugs** — plaintext wipe, log redaction build-enforced, ACK suppression (C4, H3, C3 findings)
5. **Design X3DH session protocol** (C3 milestone) — prekey bundles, key exchange message format, session state machine