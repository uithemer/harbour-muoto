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
3. **SFOS pack** (optional): copy pack `jolla/` PNGs into silica `z/icons/`, and `native/` into hicolor (`existing-only`).
4. **SFOS overlay** (optional): composite pack `overlay/` onto hicolor apps not covered by the pack.
5. **APK last** (when pack and/or overlay selected): copy pack `apk/` and/or overlay composites into `apkd-bridge/launcherIcon/` (`existing-only` for pack icons).
6. **Touch** launcher `.desktop` files (`futimens`; optional lipstick restart when APK PNGs were written and **Restart homescreen** is on).

**Native `.desktop` files are never modified** — `Icon=` stays as shipped; only hicolor PNGs behind that name change.

**Android:** pack and overlay write themed PNGs into `launcherIcon/` when stock already exists there. `apkd_launcher_*.desktop` `Icon=` lines are not modified. Restore brings back stock PNGs from backup and removes any leftover `custom/` directory from earlier betas.

**Silica** (`/usr/share/themes/sailfish-default/silica/<z>/icons/`) is updated for pack `jolla/` icons. Other stock paths under `/usr/share/themes/` are not touched.

Active theme is stored in dconf under `/apps/sailfishos-uithemer` (`activeIconPack`, `activeFontPack`, `iconOverlay`, `homeRefresh`, `wizardDone`) after a **successful** apply (GUI writes as defaultuser; C++ uses `runDconfAsDefaultUser()` only). Cover sync re-runs the full restore→backup→run→overlay cycle for the active pack.

### Live paths (where themed pixels go)

| Kind | Live path | Pack source (under `$HOME/.themepack/<pack>/` or via `/usr/share/...` symlink) |
|------|-----------|-------------|
| Jolla | `/usr/share/themes/sailfish-default/silica/<z>/icons/<icon-key>.png` | `jolla/<z>/icons/` (z cascade) |
| Native | `/usr/share/icons/hicolor/<size>/apps/<icon-key>.png` | `native/<size>/apps/` (size cascade) |
| Android | `/home/defaultuser/.local/share/apkd-bridge/launcherIcon/<key>.png` | `apk/<size>/<key>.png` (pack) or overlay composite |

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

**SFOS overlay** (step 4) composites onto hicolor only. **APK overlay** runs in step 5: composites in place on `launcherIcon/`. Only icons whose **basename is not in the pack** get overlay (`native/` ∪ `jolla/` for SFOS; `apk/` keys for Android).

Matching uses **PNG basename** on disk, not `.desktop` `Icon=` fields.

### Launcher refresh

| Kind | Mechanism |
|------|-------------|
| Native / Jolla | `futimens` on `/usr/share/applications/*.desktop`; Lipstick watches hicolor and silica icon paths |
| Android (APK) | `futimens` on `apkd_launcher_*.desktop` after PNG writes |
| Fallback | **Automatic** lipstick restart only when **Restart homescreen** (`homeRefresh`) is enabled in the apply/restore UI (main app `_finalise` after apply/restore; density restore). **Manual** restart anytime via MainPage pulley or **R** on MainPage/DensityPage (remorse, not tied to the toggle). Cover sync and helperd never restart lipstick; they only touch launcher `.desktop` mtimes |

On restore: stock PNG restore from backup, then removal of any leftover `custom/` directory.

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

Pack `jolla/<z>/icons/` is copied into live silica `z/icons/` using the same index-aligned cascade as TPS (largest z tier first for each target z). Unmapped tiers such as `z1.5-large` are valid destinations and may supply icons for smaller z targets.

### APK / apkd

Pack `apk/` PNGs are copied into `apkd-bridge/launcherIcon/<key>.png` when stock already exists there (`existing-only`). APK overlay composites onto the same path.

## Restore

`Restore theme` restores stock PNGs from the backup store (silica jolla, hicolor native, `launcherIcon/`), removes any leftover `custom/` directory, clears the backup tree, touches launchers, and sets dconf `activeIconPack` to `default` after success.

## Icon file size hints

| Asset       | Recommended size  |
| ----------- | ----------------- |
| Native app  | 172x172 (preferred), down to 86x86 |
| Jolla app   | per silica z tier in the pack |
| APK app     | 192x192 (preferred), down to 86x86 |
| Overlay     | 192x192 / 172x172 composite canvas |
