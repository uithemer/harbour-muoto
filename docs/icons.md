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
2. **Mirror** stock Jolla launcher icons (`icon-launcher-*`) from themes silica into hicolor (create-if-missing; themes are never written).
3. **Backup** current stock PNGs with *first snapshot wins* (`ignore-existing`).
4. **SFOS pack** (optional): copy themed Jolla + native PNGs into hicolor (`existing-only`).
5. **SFOS overlay** (optional): composite pack `overlay/` onto hicolor apps not covered by the pack.
6. **APK last** (when pack and/or overlay selected): copy pack `apk/` and/or overlay composites into `apkd-bridge/custom/`, then rewrite `apkd_launcher_*.desktop` `Icon=` lines.
7. **Touch** launcher `.desktop` files (`futimens`; optional lipstick restart when APK PNGs were written and **Restart homescreen** is on).

**Native `.desktop` files are never modified** — `Icon=` stays as shipped; only hicolor PNGs behind that name change.

**Android:** apkd stock PNGs stay in `launcherIcon/`. Themed pack and overlay output go to `custom/`. After the APK phase, `Icon=` on `apkd_launcher_*.desktop` uses `/custom/<key>.png` when that file exists, otherwise the absolute path to the active pack’s `apk/<size>/<key>.png`. Restore rewrites any non-`launcherIcon` APK `Icon=` back to `launcherIcon/<key>.png` and deletes `custom/`.

**`/usr/share/themes/` is never modified.** Stock Jolla artwork is read from `sailfish-default/silica` only as a mirror source.

Active theme is stored in dconf under `/apps/sailfishos-uithemer` (`activeIconPack`, `activeFontPack`, `iconOverlay`, `homeRefresh`, `wizardDone`) after a **successful** apply. Cover sync re-runs the full restore→mirror→backup→run→overlay cycle for the active pack.

### Live paths (where themed pixels go)

| Kind | Live path | Pack source (under `$HOME/.themepack/<pack>/` or via `/usr/share/...` symlink) |
|------|-----------|-------------|
| Jolla (launcher) | `/usr/share/icons/hicolor/<size>/apps/<icon-key>.png` | `jolla/<z>/icons/` (z cascade → hicolor; see z map below) |
| Native | `/usr/share/icons/hicolor/<size>/apps/<icon-key>.png` | `native/<size>/apps/` (size cascade) |
| Android (stock) | `/home/defaultuser/.local/share/apkd-bridge/launcherIcon/<key>.png` | *(apkd; not overwritten by apply)* |
| Android (themed) | `/home/defaultuser/.local/share/apkd-bridge/custom/<key>.png` | `apk/<size>/<key>.png` (pack) or overlay composite |

Stock `icon-launcher-*` PNGs are mirrored from `/usr/share/themes/sailfish-default/silica/<z>/icons/` into hicolor before backup (read-only source).

Theme packs are installed under `/usr/share/harbour-themepack-<name>/` (RPM metadata and often symlinks). **Icon PNGs are read** from `$HOME/.themepack/<harbour-themepack-name>/` first (classic TPS layout), then from resolved `/usr/share/...` paths if needed. UI Themer does not modify pack files.

### Jolla z → hicolor map

| Silica z tier (pack / stock source) | hicolor `apps` size |
|-------------------------------------|---------------------|
| z2.0 | 256x256 |
| z1.75 | 172x172 |
| z1.5 | 128x128 |
| z1.25 | 108x108 |
| z1.0 | 86x86 |
| z1.5-large | *(not a destination; may be used as cascade source only)* |

### Stock backup store

```
/usr/share/sailfishos-uithemer/backup/icons/
  native/<size>/apps/
  apk/
```

Jolla launcher icons in hicolor are included in the native hicolor backup. There is no separate `backup/icons/jolla/` tree.

Restore copies backup → live only where the live file still exists (TPS `existing-only`).

### Overlay

When enabled, UI Themer writes composited PNGs into the **same stock paths** as native/APK icons (not a separate pack subfolder). Each target gets a random `overlay/*.png` base composited on top of the current live stock PNG.

**SFOS overlay** (step 5) composites onto hicolor only. **APK overlay** runs in step 6: inner image from `launcherIcon/`, output to `custom/`. Only icons whose **basename is not in the pack** get overlay (`native/` ∪ `jolla/` for hicolor; `apk/` keys for Android).

Matching uses **PNG basename** on disk, not `.desktop` `Icon=` fields.

### Launcher refresh

| Kind | Mechanism |
|------|-------------|
| Native / Jolla | `futimens` on `/usr/share/applications/*.desktop`; Lipstick watches hicolor and resolves theme icon names |
| Android (APK) | Step 6: `Icon=` → `/custom/` (segment swap) or pack `apk/` path; then `futimens` on desktops |
| Fallback | **Automatic** lipstick restart only when **Restart homescreen** (`homeRefresh`) is enabled in the apply/restore UI (main app `_finalise` after apply/restore; density restore). **Manual** restart anytime via MainPage pulley or **R** on MainPage/DensityPage (remorse, not tied to the toggle). Cover sync and helperd never restart lipstick; they only touch launcher `.desktop` mtimes |

On restore: `revertApkDesktopsToLauncherIcon()` first, then stock PNG restore, then `removeApkCustomDir()`.

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

Pack `jolla/<z>/icons/` is copied into live hicolor `apps/` using the z→hicolor map above and the same index-aligned cascade as TPS (largest z tier first for each target z). Unmapped tiers such as `z1.5-large` are not destinations but may supply icons for smaller z targets.

### APK / apkd

Pack `apk/` PNGs are copied into `apkd-bridge/custom/<key>.png` when stock exists in `launcherIcon/` (APK phase). APK overlay uses the same `custom/` path. Desktop `Icon=` is updated in that same phase: `/launcherIcon/` → `/custom/` when `custom/<key>.png` exists, else absolute pack `apk/…/<key>.png` when the pack provides it.

## Restore

`Restore theme` reverts APK `Icon=` paths to `launcherIcon/`, deletes `custom/`, restores stock PNGs from the backup store (including `launcherIcon/` if backed up), **removes** all `icon-launcher-*.png` from hicolor `apps/` tiers, clears the backup tree, touches launchers, and sets dconf `activeIconPack` to `default` after success. Themes silica is never modified.

## Icon file size hints

| Asset       | Recommended size  |
| ----------- | ----------------- |
| Native app  | 172x172 (preferred), down to 86x86 |
| Jolla app   | per z→hicolor map above |
| APK app     | 192x192 (preferred), down to 86x86 |
| Overlay     | 192x192 / 172x172 composite canvas |
