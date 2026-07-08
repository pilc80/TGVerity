# TGVerity Code Review Findings — 2026-07-07

> Deep revision of all source files, docs, and patch. Reviewed 41 files across core lib, tests, shim, and desktop patch.

Cross-references: [[desktop-two-client-security-spec]] · [[testing-plan]] · [[roadmap]] · [[crypto-plan]]

## Critical

### C1. SessionStore: zero persistence
- **File:** `src/tgverity/session_store.h:10` "In-memory per-peer session"
- **Problem:** Replay cache and counters live only in RAM. Every restart loses them, enabling immediate replay attacks.
- **Spec conflict:** Security spec line 119: "Replay/counter cache durable across restart."
- **Fix:** Add disk-backed persistence (SQLite/level DB) or at minimum a `load()/save()` API wired by the platform adapter.

### C2. AAD under-binding
- **File:** `src/tgverity/bridge.cpp:15,97` — `_crypto.seal(plaintext, chatId)` / `_crypto.open(parsed->body, chatId)`
- **Problem:** AAD is a single `chatId` string. The security spec (line 116) and crypto plan (lines 69–82) require binding: Telegram account ids, device ids, protocol version, crypto suite, session id, packet type/id/counter.
- **Fix:** Bridge must construct a structured AAD blob from available context, not just `chatId`. The TelegramAdapter must deliver account/peer info on send and incoming.

### C3. Logger redaction is user-tunable, not build-enforced
- **File:** `src/tgverity/log.h:32` `setRedact(bool)` — default `true`, but anyone can call `setRedact(false)`
- **Problem:** Security spec line 122: "Logs hard-redact by build policy, not user preference only." A `setRedact(false)` call exposes plaintext in logs.
- **Fix:** `setRedact()` must be a no-op when `!TGVERITY_PROTOTYPE_INSECURE`. Or add a separate build-flag-gated `secureRedact()` that cannot be disabled.

### C4. ACK packets not suppressed from raw UI/notifications
- **File:** `src/tgverity/bridge.cpp:68–86` — `relay_ack` path has no `suppressRawRender`/`suppressRawNotification`/`suppressRawEdit` calls
- **Spec conflict:** Security spec line 61: "all TGVerity packet types, including ACK/malformed candidates"; Release blocker line 200: "ACK packets are not suppressed from raw UI/notifs"
- **Fix:** Call suppress hooks for `relay_ack` before processing.

## High

### H1. Desktop patch is inert — zero hook wiring
- **File:** `patches/tdesktop-tgverity.patch` — only branding + shim file + CMakeLists
- **Shim:** `tgverity_bridge_shim.cpp` provides `buildRelayText`/`buildRelayAck`/`parse`/`isTgVerityPacket` — pure codec functions. No calls to send/incoming/delete hooks anywhere in the patch.
- **Status:** Documented as "inert" in `docs/desktop-build.md:41`. All 5 hook call-sites unwired.
- **Impact:** No actual Telegram interception happens. The D4 phase target.

### H2. TdlibTelegramClient throws on every method
- **File:** `src/tgverity/tg_client_tdlib.cpp` — all 6 methods throw `std::runtime_error("... not implemented")`
- **Impact:** TDLib integration tests and D3 protocol gate are unexecutable. `docs/testing-plan.md:67` notes this as a gap.

### H3. Outbound/inbound plaintext never cleared
- **File:** `src/tgverity/bridge.h:16,24` — `OutboundMessage::plaintext`, `InboundMessage::plaintext` are `std::string` with no lifetime management
- **Problem:** After `cleanup_done`, the bridge still holds the plaintext in memory. No wipe mechanism. Memory dumps would expose secrets.
- **Fix:** Add `clearPlaintext()` on state transitions to `cleanup_done`, or use a wrapper that zeroes on destruction.

### H4. `failed` state is never reached in bridge
- **File:** `src/tgverity/state_machine.cpp:18` — `failed` reachable from any non-terminal state
- **Code gap:** `bridge.cpp:98–100` decrypt failure only logs warning; never calls `StateMachine::transition(..., MessageStatus::failed)`
- **Impact:** Auth failures leave messages stuck in `cleanup_pending` forever.

## Medium

### M1. `FakeTelegramClient` auto-binds IDs synchronously
- **File:** `src/tgverity/fake_telegram_client.cpp:24` — `onMessageIdBound` fires before `_deliver` in `sendPacketText`
- **Problem:** Tests pass because delivery and ID-binding are atomic. Real adapters have async round-trips. The bridge depends on synchronous binding for cleanup timing (`bridge.cpp:80–83`), which may not hold for real adapters.
- **Fix:** Add a test mode where delivery precedes ID-binding to verify the retry path.

### M2. Patch file bloated with binary diffs
- **File:** `patches/tdesktop-tgverity.patch` — ~4.4 MB; most is binary diffs for icons/Xcode assets
- **Impact:** Makes patch review nearly impossible in terminal/grep. The structural edits are ~200 lines; the rest is binary blobs.
- **Fix:** Store binary assets as separate files with a patch generator script, not inline binary diffs.

### M3. `session_store` counter has no restart-safe monotonic guarantee
- **File:** `src/tgverity/session_store.cpp:6–7` — `++session.counter`
- **Problem:** If a process restarts and the counter resets to 0, old packet IDs from the pre-restart session could be re-issued. Combined with C1 (no replay cache persistence), replay is immediate.
- **Fix:** Persist counter value with version/marker; check for wrap-around.

### M4. Bridge logs plaintext via `redactSecret` at `Info` level
- **File:** `src/tgverity/bridge.cpp:24–26,110–112`
- **Problem:** `redactSecret(plaintext, Logger::instance().redact())` — the redact flag is instance state, modifiable at runtime. At `Info` level, plaintext is visible if redaction is off.
- **Fix:** Use `Debug` level for plaintext references, or compile-guard the log line behind `TGVERITY_PROTOTYPE_INSECURE`.

## Low

### L1. `send()` uses `chatId` as both session key and crypto AAD
- **File:** `bridge.cpp:14,15`
- **Problem:** `_sessions.nextPacketId(chatId)` and `_crypto.seal(plaintext, chatId)` use the same string. Test doubles may share IDs with real chats.
- **Fix:** Separate session key from crypto key material, or derive both from a structured context.

### L2. `TGVERITY_PROTOTYPE_INSECURE` compile definition is `PUBLIC`
- **File:** `CMakeLists.txt:35`
- **Problem:** Propagates to every linking target. A downstream app could check the macro and make decisions about its own security behavior.
- **Fix:** Use `PRIVATE` unless consumers need to know.

### L3. `insecure_release_build_blocked` test naming is counter-intuitive
- **File:** `CMakeLists.txt:64–70`
- **Problem:** The test name says "fails" and `WILL_FAIL TRUE` means "we expect the command to fail, and that's a pass." Easy to misread in CI logs.
- **Fix:** Rename to `insecure_release_build_blocked` and add `message(STATUS "...")` so CI output is unambiguous.

### L4. Core parser accepts empty body for `relay_text`; shim rejects it
- **File:** `desktop/tgverity_bridge_shim.cpp:166` — `if (p.type == PacketType::RelayText && !haveBodySafe) return std::nullopt;`
- **Core:** `src/tgverity/relay_packet.cpp:108` — does NOT check `haveBodySafe`
- **Problem:** Core accepts `relay_text` with empty body; shim rejects it. Fine for v0.2 (IdentityCryptoProvider produces same-size output), but will break with real crypto (sealed output is never empty).
- **Fix:** Add `haveBodySafe` check to core parser.

### L5. `v0.2-design.md` references stale source paths
- **File:** `docs/v0.2-design.md:75`
- **Problem:** `api_sending`, `api_updates`, `data_histories` — upstream may have moved these between commits. Patch applied successfully, so paths match the *current* patch, but the design doc references may not match future commits.
- **Fix:** Pin doc paths to the patch's source commit, or remove.

### L6. Bridge uses `chatId` as both session key and crypto AAD
- **File:** `src/tgverity/bridge.h:54–57`, `bridge.cpp:14,15`
- **Problem:** `_sessions.nextPacketId(chatId)` and `_crypto.seal(plaintext, chatId)` share one string. Test doubles may collide with real chats. Does not scale to multi-peer sessions.
- **Fix:** Derive session key and crypto key material from structured context, not raw `chatId`.

### L7. No compile-time detection of stale shim/core drift
- **File:** `desktop/tgverity_bridge_shim.cpp`, `src/tgverity/relay_packet.cpp`
- **Problem:** Base32 codec and envelope format are duplicated. A future change in one silently diverges the other. L4 (empty-body mismatch) is an example of existing drift.
- **Fix:** C ABI bridge (planned) eliminates duplication; until then, add a compile-time format-version test that both modules must pass.