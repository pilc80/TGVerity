# TGVerity Platform Adapter Contract — 2026-07-05 20:45 MSK

## Purpose

Keep `tgverity-core` portable while each Telegram client supplies only transport/UI/storage hooks.

```text
Telegram client patch/hooks
        ↓
tgverity-platform-adapter
        ↓
tgverity-core
```

## Boundary rule

Telegram-side code may call adapter methods. It must not inspect TGVerity internals.

| Must stay inside `tgverity-core` | May exist in adapter/platform |
|---|---|
| identity private keys | Telegram account/chat/message ids |
| session keys / ratchet state | send/delete APIs |
| decrypted plaintext storage | render/notification suppression hooks |
| packet replay cache | UTF-16/entity conversion |
| verification transcript | QR/camera/platform UI bridge |
| crypto provider | platform secure-storage implementation |

## Minimal adapter API

```text
sendPacketText(chat_id, packet_text, send_token) -> platform_send_id
onMessageIdBound(platform_send_id, server_msg_id)
onIncomingText(chat_id, server_msg_id, text, entities)
deleteMessagesRevoke(chat_id, server_msg_ids)
suppressRawRender(server_msg_id)
suppressRawNotification(server_msg_id)
suppressRawEdit(server_msg_id)
renderVirtualMessage(chat_id, virtual_msg)
showTrustState(chat_id, state)
secureStoreGet/Put/Delete(namespace, key, value)
```

## Send flow

```text
user plaintext
  ↓
core verifies chat mode + trust state
  ↓
core encrypts/builds packet
  ↓
adapter sends Telegram text packet
  ↓
platform binds send id → server msg id
  ↓
core updates local_pending → tg_sent
```

## Receive flow

```text
incoming Telegram text
  ↓
adapter detects TGVerity prefix only as a candidate
  ↓
core authenticates/decrypts/replay-checks
  ↓
valid: suppress raw render/notification, store virtual msg, ACK
invalid: no plaintext, optional warning, no Safe upgrade
```

## Required platform hooks

| Hook | Purpose | Required for MVP |
|---|---|---:|
| **S send wrap** | replace TGVerity plaintext send with packet send | ✅ |
| **B id bind** | bind local/random/send id to server msg id | ✅ |
| **I incoming text** | detect/auth/decrypt packet candidates | ✅ |
| **D delete revoke** | best-effort cleanup of raw MSG/ACK | ✅ |
| **N notification suppress** | prevent raw packet preview | ✅ |
| **R render suppress** | prevent raw packet bubble | ✅ |
| **E edit suppress** | raw/TGVerity messages immutable in MVP | ✅ |
| **Q QR/camera** | identity verification | ✅ for product MVP |
| **K secure storage** | keys/session state | ✅ before real crypto release |

## Per-platform strategy

| Platform | Preferred strategy | Notes |
|---|---|---|
| **TDLib CLI** | library adapter | regression harness; no native UI |
| **Telegram Desktop** | fork patch + shim now; C ABI later | current macOS proof path |
| **Android** | fork patch | primary MVP; needs hook map before code |
| **iOS** | feasibility first | notification extension/storage/App Store constraints |
| **Web** | later library/patch | weaker local security; useful for research/headless |

## Data handling rules

| Rule | Reason |
|---|---|
| Do not pass plaintext to Telegram send APIs | prevents cloud/search/history leaks |
| Do not log packet bodies, plaintext, keys, or safety numbers | prevents local telemetry leaks |
| Disable link previews for packet text where possible | avoids server-side preview processing |
| Generate entities with correct UTF-16 offsets | Telegram entity offsets are UTF-16 |
| Cleanup uses revoke/delete-for-all first | self-delete can make revoke impossible |
| Cleanup failure never invalidates TGVerity delivery | deletion is hygiene, not security |

## Adapter acceptance tests

Every platform adapter must pass:

1. Normal Telegram message sends unchanged.
2. TGVerity send produces packet text, not plaintext.
3. Incoming valid packet renders only as virtual TGVerity message.
4. Incoming invalid packet never renders plaintext and never upgrades trust.
5. Raw packet notification is suppressed.
6. Raw edit affordance is suppressed.
7. ACK moves sender state to `tgverity_ack`.
8. Cleanup calls revoke/delete-for-all once with known ids.
9. Key/session material never crosses into Telegram-side logging/UI code.

## Release blockers

```text
Adapter exposes plaintext to Telegram message/search/notification paths.
Adapter can show Safe without core verification state.
Adapter deletes self-only before revoke/delete-for-all.
Adapter logs packet body, plaintext, keys, or verification secrets.
Adapter accepts prefix-only packets as trusted.
```
