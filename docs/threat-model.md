# TGVerity Threat Model — 2026-07-05 20:19 MSK

## Purpose

```text
TGVerity is not a new global messenger.
TGVerity is a verified secure overlay for Telegram-compatible clients.
```

It protects message content and trust labels from Telegram transport assumptions. It does **not** hide every metadata signal and does **not** protect a compromised endpoint.

## Assets

| Asset | Must protect from |
|---|---|
| TGVerity plaintext | Telegram servers, network observers, unsupported clients |
| Session keys / ratchet state | Telegram-side code, logs, backups, UI layers |
| Identity keys | server-side substitution, accidental reset, silent replacement |
| Verification state | MITM, key-change confusion, downgrade UI |
| Packet integrity | relay tamper/edit/replay/copy-to-chat |
| Local virtual history | raw Telegram packet history, search/notification leaks |

## Adversaries

| Adversary | Capability | TGVerity stance |
|---|---|---|
| **Telegram cloud/server operator** | sees cloud sender, recipient, timing, size, packet text; may store/delete/rate-limit relay packets | cannot read TGVerity plaintext if crypto is real; metadata still visible in Relay |
| **Government / military / intelligence request to Telegram** | may obtain Telegram-side stored cloud data and metadata | Relay content should remain opaque; Telegram metadata remains available |
| **Network observer / ISP** | sees IP traffic, timing, endpoints | Telegram TLS hides packet body from ISP; TGVerity does not hide Telegram usage |
| **MITM / key-substitution attacker** | tries to replace TGVerity identity/session keys | must be detected by QR/safety-number verification before `Safe` label |
| **Malicious peer** | screenshots, forwards plaintext manually, lies about identity | cannot be solved cryptographically after peer decrypts |
| **Compromised device** | reads screen, memory, filesystem, notifications | out of scope except local-storage and notification hygiene |
| **Upstream Telegram client change** | adds logging/telemetry near secure paths or breaks hooks | release blocker |

## Guarantees TGVerity may claim

| Guarantee | Condition |
|---|---|
| **Telegram cannot read TGVerity message content** | only after real audited E2EE replaces IdentityCryptoProvider |
| **MITM is detectable** | verification binds Telegram account, TGVerity identity key, device id, protocol version, session challenge |
| **No silent downgrade** | secure-session state machine refuses to relabel normal Telegram as Safe |
| **Packet tamper/replay is rejected** | AEAD/AAD + packet id/counter/replay cache are active |
| **Relay cleanup is best-effort hygiene** | never described as security deletion |

## Non-guarantees

| Non-guarantee | Reason |
|---|---|
| **Metadata privacy in Telegram Relay** | Telegram still sees accounts, timing, frequency, approximate size, delivery behavior |
| **No listener exists anywhere** | servers, networks, devices, and peers may observe metadata or plaintext after endpoint decrypt |
| **Serverless communication** | Relay uses Telegram infrastructure; Direct can fail |
| **Protection from compromised devices** | malware/rooted OS/screen capture can observe plaintext |
| **Protection from malicious recipients** | recipient can copy or disclose decrypted text |
| **Deleted means gone** | Telegram/server/client copies may persist; cleanup is best-effort only |

## Trust labels

| Label | Required condition |
|---|---|
| **Telegram Cloud** | normal Telegram chat; no TGVerity protection |
| **TGVerity Encrypted — Unverified** | encrypted session exists, but identity not verified |
| **TGVerity Safe Relay** | identity verified; Telegram carries opaque TGVerity packets |
| **TGVerity Safe Direct** | identity verified; direct transport active |
| **Key changed — reverify** | previous identity no longer matches; never show Safe |

Forbidden words unless narrowly qualified: `secure Telegram`, `serverless`, `military-grade`, `anonymous`, `metadata-private`, `government-proof`, `untraceable`.

## Required architecture constraints

```text
Telegram account is an address, not a trust root.
TGVerity identity key is the trust root.
Verification binds account + key + device + protocol version + session challenge.
Relay mode protects content, not metadata.
Direct mode reduces Telegram transport visibility, but does not solve endpoint compromise.
```

| Constraint | Implication |
|---|---|
| Independent TGVerity identity | do not derive trust solely from Telegram account/session |
| Per-device identity for MVP | multi-device is explicit later work, not silent key sharing |
| Narrow platform adapter | Telegram-owned raw message/search/history/notification/storage paths never receive plaintext; TGVerity-owned render code may receive transient display plaintext |
| Open packet/protocol docs | security audience must inspect claims |
| External crypto review before release claims | v0.x prototypes must stay labeled insecure until reviewed |

## Release blockers

A release must stop if any of these are true:

```text
IdentityCryptoProvider is enabled in a public security build.
Safe can be shown before verification.
Key change can be ignored or silently accepted.
Plaintext enters Telegram message text/search/notification paths.
Relay metadata is described as hidden.
Raw cleanup is described as guaranteed deletion.
Upstream Telegram changes add logging/telemetry around secure paths.
```
