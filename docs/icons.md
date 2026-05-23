---
layout: default
title: Icon pack guidelines
parent: Get started
nav_order: 1
---

# Icon pack guidelines

How to create icons compatible with UI Themer.

## How icons are applied (TPS model)

UI Themer reimplements the classic Theme Pack Support icon pipeline:

1. **Restore** stock PNGs from `backup/icons/` (if a backup exists).
2. **Backup** current stock PNGs with *first snapshot wins* (`ignore-existing`).
3. **Run** the pack: copy themed PNGs into live system trees (`existing-only`).
4. **Overlay** (optional): composite pack `overlay/` onto apps not covered by the pack.
5. **Touch** launcher `.desktop` files so Lipstick reloads icons.

**`.desktop` files are never modified.** `Icon=` stays as shipped; only the PNG files behind that name change.

Active theme is stored in dconf (`activeIconPack`) after a **successful** apply. Cover sync re-runs the full restore→backup→run→overlay cycle for the active pack.

### Live paths (where themed pixels go)

| Kind | Live path | Pack source |
|------|-----------|-------------|
| Native | `/usr/share/icons/hicolor/<size>/apps/<icon-key>.png` | `native/<size>/apps/` (cascaded to smaller sizes) |
| Jolla (ambient) | same hicolor paths | `jolla/<z>/icons/` (scaled into hicolor, index-aligned z cascade) |
| Android | `/home/defaultuser/.local/share/apkd-bridge/launcherIcon/<key>.png` | `apk/<size>/<key>.png` |

Theme packs under `/usr/share/harbour-themepack-<name>/` are **read-only**; UI Themer does not modify them.

### Stock backup store

```
/usr/share/sailfishos-uithemer/backup/icons/
  native/<size>/apps/
  apk/
```

Restore copies backup → live only where the live file still exists (TPS `existing-only`).

### Overlay

When enabled, UI Themer writes composited PNGs into the **same stock paths** as native/APK icons (not a separate pack subfolder). Apps without a matching pack PNG get a random `overlay/*.png` base plus the current stock icon.

- **Native:** live hicolor icons whose basename is not in the pack (`native/` ∪ `jolla/`).
- **APK:** live `launcherIcon` icons with no matching `apk/<size>/<basename>.png`.

### Launcher refresh

After apply or restore, UI Themer bumps mtimes on launcher `.desktop` files (Clockwork-style `futimens`). If the user enabled **Restart homescreen (fallback)** in settings, `lipstick.service` is restarted for defaultuser.

Concurrent icon, restore, font, or density operations are rejected via `flock` on `/usr/share/sailfishos-uithemer/icon-ops.lock` (`busy`).

> Graphic widget PNG theming has been **removed** since 2.4.2.

## Theme pack layout

Inside `/usr/share/harbour-themepack-<name>/`:

```
native/
  256x256/apps/<icon-key>.png
  172x172/apps/<icon-key>.png
  ...

jolla/
  z2.0/icons/<icon-key>.png
  ...

apk/
  192x192/<launcher_id>.png
  ...

overlay/
  *.png
```

### Matching (apply and match counts)

Icons are matched by **PNG basename** on disk (classic TPS). UI Themer does not read `.desktop` `Icon=` fields.

A native icon is themed when the pack has `<key>.png` and stock already has `hicolor/*/apps/<key>.png` (`existing-only`).

### Z → hicolor (jolla apply)

Pack `jolla/<z>/icons/` uses the same index-aligned cascade as TPS (largest z tier first), but pixels are **scaled** into existing hicolor sizes:

| Index | Hicolor | Pack z |
|-------|---------|--------|
| 0 | 256×256 | z2.0 |
| 1 | 172×172 | z1.75 |
| 2 | 128×128 | z1.5-large |
| 3 | 108×108 | z1.25 |
| 4 | 86×86 | z1.0 |

If the pack also has `native/<size>/apps/<key>.png`, native wins for that key.

### APK / apkd

Largest PNG folder under `<pack>/apk/` is copied into flat `apkd-bridge/launcherIcon/<key>.png` when the live file exists. After writes, PNGs are owned by `defaultuser`.

## Restore

`Restore theme` restores stock PNGs from the backup store, clears the backup tree, touches launchers, and sets dconf `activeIconPack` to `default` after success.

## Icon file size hints

| Asset       | Recommended size  |
| ----------- | ----------------- |
| Native app  | 172x172 (preferred), down to 86x86 |
| APK app     | 192x192 (preferred), down to 86x86 |
| Overlay     | 192x192 / 172x172 composite canvas |
