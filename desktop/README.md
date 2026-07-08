# TGVerity Desktop Bridge (v0.2)

Standalone shim that lets a patched Telegram Desktop build speak the TGVerity
v1 packet format without linking `tgverity-core`.

## Shim purpose

`tgverity_bridge_shim.{h,cpp}` mirror the core packet format **byte-for-byte**
so a packet built by `tgverity-core` round-trips through the shim and vice
versa. Build + parse + predicate, zero external deps, C++17.

Authoritative format references:

- `docs/v0.2-design.md` (packet format + state machine)
- `src/tgverity/relay_packet.cpp` (codec + envelope logic)
- `desktop/tgverity_bridge_shim.h` (format contract comment)

```text
wire_text = kPrefix + base32(envelope)

envelope ("\n"-separated, this key order):
  protocol=tgverity
  version=1
  crypto_suite=0
  type=relay_text | relay_ack
  packet_id=<counter>
  ref_id=<id|"-">
  body_safe=base32(provider_output_bytes)

kPrefix   = "TGVerity secure packet. Open with TGVerity to read.\nv1:"
alphabet  = A B C D E F G H J K M N P Q R S T V W X Y Z 2 3 4 5 6 7 8 9 0 1
            (no I, L, O, U) — same bit-packing as relay_packet.cpp
```

> Format-mirror disclaimer: the base32 codec and envelope assembly are
> **duplicated** from `tgverity-core` so the shim compiles inside the tdesktop
> tree with no link dependency. This is intentional for v0.2; see the C ABI plan
> below.

## Self test

```sh
g++ -std=c++17 desktop/tgverity_bridge_shim.cpp \
                desktop/tgverity_bridge_shim_tests.cpp \
                -o /tmp/tgv_shim_test && /tmp/tgv_shim_test
```

Asserts: body round-trip (incl. NUL/0xff); `isTgVerityPacket` predicate; wire
contains no `http`, no `@`, no `#`; ACK carries `ref_id`; empty body round-trip.
Expected output: `PASS: all tgverity_bridge_shim tests`, exit 0.

## Hook wiring (5 points)

Mapped to `docs/message-lifecycle.md` "Telegram Desktop hook map". The DRAFT
patch lives at `patches/tdesktop-tgverity-hooks-v0.2.DRAFT.patch`.

| # | Need | tdesktop file | Shim call | Intent |
|---|---|---|---|---|
| 1 | send-wrap | `api/api_sending.cpp` | `buildRelayText(text, packetId)` | Replace outgoing plaintext with wire text for TGVerity-enabled chats; keep `random_id` for the id bind |
| 2 | incoming | `api/api_updates.cpp` (`updateNewMessage`) | `isTgVerityPacket` + `parse` | Detect packet, hand body to adapter, mark item for render/notif suppress |
| 3 | id-bind | `api/api_updates.cpp` (`updateMessageID`) | — (callback only) | Bind `random_id` -> server `msg_id` so core moves `local_pending` -> `tg_sent` |
| 4 | delete-revoke | `data/data_histories.cpp` (`deleteMessages` revoke=true) | — (policy guard) | One revoke call, batch <=100 ids, never self-then-revoke, skip undeletable |
| 5 | notif suppress | `history/history_item.cpp` (`skipNotification`) | `isTgVerityPacket` | Raw packet never notifies; adapter emits controlled virtual notification |
| 6 | edit suppress | `history/view/history_view_context_menu.cpp` | `isTgVerityPacket` | TGVerity messages immutable; hide Edit affordance |

Each hook point is tagged `// TGVHOOK v0.2: <intent>` and wrapped in
`// TGVHOOK BEGIN` / `// TGVHOOK END` so a human finalizes placement post-checkout.

## C ABI plan (replaces duplication) — BLOCKED

The duplicated codec/envelope is temporary. Once the GUI build path is stable,
expose `tgverity-core` via a thin C ABI and drop the duplicate.

**Blocks:** D3/D4 Desktop hook wiring. The C ABI is a prerequisite for H1 (hook wiring) because the shim's current standalone codec makes drift detection impossible. Until the C ABI is implemented, the shim is the single source of truth for the wire format — and any divergence silently breaks cross-module round-trips.

Priority: C ABI bridge → H1 hook wiring → D4 build attempt.

```c
// tgverity_core.h (planned)
size_t tgv_build_relay_text(const char* body, size_t body_len,
                            uint64_t packet_id, char* out, size_t out_cap);
int     tgv_is_packet(const char* text, size_t text_len);
int     tgv_parse(const char* text, size_t text_len,
                  tgv_parsed_t* out);   // type, packet_id, ref_id, body bytes
```

The shim's public API (`buildRelayText` / `parse` / `isTgVerityPacket`) stays
the same; only the implementation swaps to forward to the C ABI. Zero change to
the 5 hook call sites.

## Branding patch

`patches/tdesktop-macos-tgverity-v0.1.patch` is **retained as-is** for v0.2
(bundle id, app name, icons, lang strings, macOS global menu). Apply it before
the v0.2 hook patch; the two touch disjoint files except `Telegram/CMakeLists.txt`,
where v0.1 edits the project/bundle block and v0.2 only appends `target_sources`
for the shim — no conflict expected, re-verify on apply.

## Build / link notes

- Shim TU is standalone; copy (or symlink) `desktop/tgverity_bridge_shim.{h,cpp}`
  into the tdesktop tree at `Telegram/SourceFiles/tgverity/` and add to
  `target_sources(Telegram PRIVATE ...)` (see the CMakeLists hunk in the DRAFT patch).
- No Qt types in the shim API; hook sites do the `QString::fromStdString` /
  `.toStdString()` marshalling.
- No logging from the shim on purpose (keep it dependency-free); tdesktop-side
  hooks log via the existing `Core::App` logger once wired to the adapter.
- Packet size cap: wire text must fit Telegram's 4096-char message limit
  (prefix + base32 envelope). The shim does not enforce it; the send-wrap hook
  must split or fall back to file transport for oversized bodies.
