# TGVerity Desktop Build — 2026-07-05 MSK

Reproducible build for the TGVerity desktop client (Telegram Desktop fork).

## Source of truth

- **Our code:** this repo — `tgverity-core` lib (`src/`) + `desktop/tgverity_bridge_shim.*`.
- **Desktop integration:** `patches/tdesktop-tgverity.patch` — branding + icons + libheif fix + shim + window-title, vs tdesktop commit `85c94951ab`.
- **Session:** `~/Library/Application Support/TGVerity/tdata` (keyed on `AppName=TGVerity`, not the binary path).

## Build (macOS arm64, MinSizeRel)

```text
mkdir -p .build
git clone --recursive https://github.com/telegramdesktop/tdesktop.git .build/tdesktop-src
cd .build/tdesktop-src && git checkout 85c94951ab
git apply /path/to/TGVerity/patches/tdesktop-tgverity.patch
cd Telegram
./build/prepare/mac.sh                       # long; keep all TGVerity scratch under .build/ when configurable
./configure.sh -D TDESKTOP_API_ID=2040 -D TDESKTOP_API_HASH=b18441a1ff607e10a989891a5462e627
cd ..
xcodebuild -project out/Telegram.xcodeproj -scheme Telegram \
    -configuration MinSizeRel -destination 'platform=macOS,arch=arm64' \
    ONLY_ACTIVE_ARCH=YES build
```

Product: `out/MinSizeRel/TGVerity.app`. Runnable copy kept at `/Applications/TGVerity.app`.

## Hard rules (see memory: desktop-branding-hard-rule)

- `AppName=TGVerity` always — it controls the `TGVerity/tdata` path = the logged-in session. **Never build unbranded** (orphans the session, forces relogin).
- Keep `AppVersionStr=6.9.4` (Telegram-protocol version sent to servers). TGVerity product version `0.2` is user-facing only (window title).
- libheif dep fix (`WITH_GDK_PIXBUF=OFF`, `WITH_DAV1D=OFF` if the x86_64 slice fails on arm64 Homebrew) is in the patch.

## Creds

`TDESKTOP_API_ID=<id>`, `TDESKTOP_API_HASH=<hash>`. Use credentials allowed by the upstream/build policy; do not commit private credentials.

## Status

- Shim compiles in-tree; **inert** — the 5 hook call-sites (send-wrap / incoming / delete-revoke / notif / edit) are not wired yet.
- Branding + icons + libheif + title: applied + validated (build green, runs logged in).
