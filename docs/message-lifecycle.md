# TGVerity Message Lifecycle — 2026-07-04 20:18 MSK

## Goal

```text
Telegram history  = disposable opaque transport queue
TGVerity history  = real decrypted virtual messages
Cleanup           = best-effort hygiene, not security
```

## Carrier choice

```text
Telegram Secret Chats = MTProto E2EE, but single-device, no multi-device sync
TGVerity relay        = own E2EE (X3DH/Double-Ratchet) carried in cloud text
```

Cloud text is **not** E2EE by default (Telegram holds keys), so TGVerity encrypts the payload itself → content privacy + multi-device + verified identity, distinct from both cloud and Secret Chats. Secret chats remain a separate, non-TGVerity trust state.

## Architecture

```text
Telegram client patch/hooks
        │
        ▼
tgverity-platform-adapter   # peer ids, send/delete APIs, notifications, UI glue
        │
        ▼
tgverity-core               # sessions, crypto, packets, ACK/read state, storage
```

| Layer | Portable? | Owns |
|---|---:|---|
| `tgverity-core` | yes | packet format, crypto/session state, ACK/read/delete state |
| `tgverity-platform-adapter` | per platform | Telegram ids, send/delete hooks, notification/UI calls |
| Telegram patches | per client | smallest hook calls only |

## Adapter strategies

```text
Fork strategy  = patch official client (Android/iOS/Desktop) → native UX, high upstream-tracking cost
TDLib strategy = thin client over TDLib                       → clean adapter, no fork, rebuild UI
```

Prefer **TDLib** for non-primary platforms; reserve the **fork for primary mobile** (native UX matters there). All platforms expose the same MTProto primitives, so `tgverity-core` stays identical across both strategies.

## Packet text

```text
TGVerity:
<spoiler entity>opaque_payload</spoiler>
```

| Part | Purpose |
|---|---|
| `TGVerity:` | parser anchor + unsupported-client label |
| single spoiler entity | structural UX-only hide until tapped; **server still sees text** — not security |
| payload | encrypted + authenticated envelope |

Payload carries **zero** formatting/link/mention entities (reconciles README rule 5); the spoiler is one structural wrapper, not formatting. Whole message ≤ **4096 chars** incl. prefix+payload; larger → multi-msg/file. `pre`/code also suppresses server auto-URL detection but adds a copy affordance — spoiler preferred for MVP.

## Detection

```text
incoming Telegram text
  │
  ├─ chat not TGVerity-enabled → do nothing
  ├─ missing TGVerity prefix   → do nothing
  ├─ malformed/too large       → normal or warning, no decrypt
  ├─ auth/decrypt fails        → suspicious packet, no plaintext
  └─ valid packet              → consume as TGVerity
```

| Attack | Defense |
|---|---|
| fake prefix | require enabled chat + authenticated envelope |
| copy to another chat | bind payload to Telegram peer/account/session |
| replay | packet id / counter replay cache |
| edit/tamper | AEAD/auth failure |
| fake ACK | ACK authenticates and references known packet/msg id |

## MSG send flow

```text
A user sends plaintext
  │
  ├─ TGVerity off → normal Telegram send
  │
  └─ TGVerity on
       │
       ├─ core creates MSG packet + virtual msg(local_pending)
       ├─ adapter sends opaque Telegram text
       ├─ hook: bind client send-id → server msg_id
       │    ├─ TDLib: updateMessageId(local → server)
       │    └─ MTProto/tdesktop: updateMessageID(random_id → server)
       ├─ core binds packet_id ↔ send-id ↔ msg_id → tg_sent
       ├─ hook: incoming ACK packet
       │    └─ core marks tgverity_ack
       └─ adapter deletes raw MSG + raw ACK best-effort
```

## MSG receive flow

```text
B receives Telegram text
  │
  ├─ not TGVerity packet → normal Telegram render/notify
  │
  └─ valid TGVerity MSG
       │
       ├─ suppress raw render + raw notification
       ├─ core decrypts/authenticates/stores virtual msg
       ├─ adapter renders TGVerity bubble
       ├─ core creates ACK(ref_packet_id)
       ├─ adapter sends ACK packet
       └─ after ACK send confirmed: delete raw MSG + raw ACK best-effort
```

## ACK cleanup

```text
A sends MSG ─────────────────────────▶ B
A binds msg_id                         B decrypts + stores
                                       B sends ACK
A receives ACK ◀────────────────────── B binds ack_id
A deletes MSG + ACK                    B deletes MSG + ACK
```

| Side | Delete MSG raw when | Delete ACK raw when |
|---|---|---|
| sender | ACK authenticated + MSG `msg_id` known | ACK processed + ACK `msg_id` known |
| receiver | MSG stored + ACK send confirmed | ACK send confirmed + ACK `msg_id` known |

No ACK-of-ACK for MVP. Failed cleanup never invalidates the TGVerity message.

## Status model

| State | Meaning |
|---|---|
| `local_pending` | virtual message created |
| `tg_sent` | Telegram accepted raw packet |
| `tgverity_ack` | peer TGVerity stored/decrypted |
| `cleanup_pending` | raw MSG/ACK delete still needed |
| `cleanup_done` | delete attempted/succeeded |
| `failed` | send/decrypt/ACK failed |

## Notifications

```text
raw packet notification      → suppress
virtual TGVerity notification → show controlled TGVerity text
```

| Case | Notification |
|---|---|
| valid verified MSG | TGVerity message from peer; preview only if allowed |
| valid unverified MSG | TGVerity encrypted message — unverified |
| invalid packet | no raw preview; optional in-chat warning |
| other logged-in devices | show opaque ciphertext (`TGVerity:`+blob) preview — UX leak, not plaintext |

Push previews ride in FCM/APNs as an encrypted `"p"` param; the **client decrypts + renders it**, so TGVerity's own client controls the preview → can show *"TGVerity message"* instead of raw packet. Requires patching the iOS Notification-Service-Extension and Android FCM data-service path, else raw packet previews briefly.

## Editing/deleting

| Action | Rule |
|---|---|
| edit TGVerity msg | disabled; send correction as new MSG |
| raw packet edit | reject on receive; AEAD/auth fails |
| delete TGVerity msg | local virtual delete + best-effort raw cleanup |
| Telegram delete API | use revoke/delete-for-all where allowed |

## Telegram Desktop hook map

Paths/symbols valid on `dev`; **re-pin per upstream commit** at integration (e.g. `registerSentMessageRandomIds`, not legacy `registerMessageRandomId`).

| Need | Source hook area |
|---|---|
| outgoing send wrap | `history/history_widget.cpp`, `api/api_sending.cpp` |
| random id → msg id | `api/api_updates.cpp` `updateMessageID`, `data_session.cpp::registerMessageRandomId` |
| incoming packets | `api/api_updates.cpp` `updateNewMessage`, `data_session.cpp::addNewMessage` |
| cleanup delete | `data/data_histories.cpp::deleteMessages(..., revoke)` |
| notifications | `window/notifications_manager.cpp`, `HistoryItem::skipNotification()` |
| edit menu | `history/view/history_view_context_menu.cpp`, `history_widget.cpp::saveEditMessage` |

## Platform adapter coverage

Hooks per platform: S=send wrap · B=bind send-id→msg_id · I=incoming · D=delete revoke · N=notif · E=edit · R=entities(UTF-16).

| Platform | Source | Strategy | S B I D N E R | Verdict | MVP |
|---|---|---|---|---|---|
| TDLib | tdlib/td | lib, no fork | ✅✅✅✅✅✅✅ | cleanest | **v0.1 ✓** |
| Desktop | tdesktop | fork *or* TDLib | ✅✅✅✅✅✅✅ | high | fork now / TDLib ✓ |
| Android | DrKLO/TMessagesProj | fork | ✅ `SendMessagesHelper`/`MessagesController` | high | **primary** |
| iOS | Telegram-iOS | fork *or* TDLib | ✅ Postbox/TelegramCore | med-high (2M LOC, App Store) | later |
| Web | gramJS webK/webZ | lib/patch | ✅ `addEventHandler`+`invoke` | high | later |
| MTProto libs | Telethon/Pyrogram/gramJS/MTKruto | lib | ✅ trivial | high (headless) | testing/relay |

All platforms clear the hook set; only maintenance cost differs. Symbols illustrative — re-pin per upstream commit.

## Telegram/TDLib facts

| Fact | Impact |
|---|---|
| `messages.deleteMessages(revoke=true)` deletes for all; in 1:1 via user client **no hard time limit** (48h cap is Bot API only) | cleanup can target MSG+ACK any time |
| revoke must be the **first/only** delete; cannot revoke a message already deleted-for-self | cleanup ordering: one revoke call, never self-then-revoke |
| revoke `MESSAGE_DELETE_FORBIDDEN` for service msgs; check `messageProperties.can_be_deleted_*` per msg | cleanup skips undeletable ids, never assumes success |
| revoke **forced true** for supergroups/channels/secret chats; batch limit **≤100 ids/call** | cleanup batches ids; MVP is 1:1 so no admin gating |
| TDLib `deleteMessages(chat_id, ids, revoke)` same model; `link_preview_options{is_disabled:true}` kills previews | CLI/mobile adapters share policy + preview hygiene |
| Telegram spoiler entity exists (`messageEntitySpoiler` / `textEntityTypeSpoiler`) | payload can be spoilered for UX hygiene |
| entity offsets use UTF-16 units | adapter must generate entities carefully |
| text cap **4096 chars** total (caption 1024) | packet must fit prefix+payload in 4096; larger → multi-msg/file |

## MVP invariants

```text
1. Normal Telegram chats are untouched.
2. Prefix alone is never trusted.
3. Plaintext never enters Telegram message text/search/notification paths.
4. TGVerity ACK, not Telegram read, means delivered/stored.
5. Cleanup is retried/best-effort and never a security claim.
6. Platform patches call adapter hooks only; protocol stays in tgverity-core.
7. Raw cleanup uses revoke (delete-for-all) as the single delete; never self-then-revoke.
```
