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
3. **Run** the pack (optional): copy themed PNGs into live system trees (`existing-only`).
4. **Overlay** (optional): composite pack `overlay/` onto apps not covered by the pack.
5. **Touch** launcher `.desktop` files so Lipstick reloads icons.

**`.desktop` files are never modified.** `Icon=` stays as shipped; only the PNG files behind that name change.

Active theme is stored in dconf (`activeIconPack`) after a **successful** apply. Cover sync re-runs the full restore→backup→run→overlay cycle for the active pack.

### Live paths (where themed pixels go)

| Kind | Live path | Pack source (under `$HOME/.themepack/<pack>/` or via `/usr/share/...` symlink) |
|------|-----------|-------------|
| Jolla (ambient) | `/usr/share/themes/sailfish-default/silica/<z>/icons/<icon-key>.png` | `jolla/<z>/icons/` (z cascade) |
| Native | `/usr/share/icons/hicolor/<size>/apps/<icon-key>.png` | `native/<size>/apps/` (size cascade) |
| Android | `/home/defaultuser/.local/share/apkd-bridge/launcherIcon/<key>.png` | `apk/<size>/<key>.png` |

Theme packs are installed under `/usr/share/harbour-themepack-<name>/` (RPM metadata and often symlinks). **Icon PNGs are read** from `$HOME/.themepack/<harbour-themepack-name>/` first (classic TPS layout), then from resolved `/usr/share/...` paths if needed. UI Themer does not modify pack files.

### Stock backup store

```
/usr/share/sailfishos-uithemer/backup/icons/
  jolla/<z>/icons/
  native/<size>/apps/
  apk/
```

Restore copies backup → live only where the live file still exists (TPS `existing-only`).

### Overlay

When enabled, UI Themer writes composited PNGs into the **same stock paths** as native/APK icons (not a separate pack subfolder). Each target gets a random `overlay/*.png` base composited on top of the current live stock PNG.

Overlay runs on **both** native hicolor and APK `launcherIcon` trees. Only live icons whose **basename is not in the pack** get composited (`native/` ∪ `jolla/` for hicolor, `apk/<size>/` for Android). Apps the theme already provides an icon for are left unchanged.

Matching uses **PNG basename** on disk, not `.desktop` `Icon=` fields.

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

### Z cascade (jolla apply)

Pack `jolla/<z>/icons/` is copied into live `silica/<z>/icons/` using the same index-aligned cascade as TPS (largest z tier first for each target z).

### APK / apkd

Largest PNG folder under `<pack>/apk/` is copied into flat `apkd-bridge/launcherIcon/<key>.png` when the live file exists. After writes, PNGs are owned by `defaultuser`.

## Restore

`Restore theme` restores stock PNGs from the backup store, clears the backup tree, touches launchers, and sets dconf `activeIconPack` to `default` after success.

## Icon file size hints

| Asset       | Recommended size  |
| ----------- | ----------------- |
| Native app  | 172x172 (preferred), down to 86x86 |
| Jolla app   | per z tier under `silica/` |
| APK app     | 192x192 (preferred), down to 86x86 |
| Overlay     | 192x192 / 172x172 composite canvas |
