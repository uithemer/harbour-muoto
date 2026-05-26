---
layout: default
title: Icon pack guidelines
parent: Get started
nav_order: 1
---

# Icon pack guidelines

How to create icons compatible with **UI Themer**.

## How icons are applied

UI Themer uses the classic Theme Pack Support icon pipeline:

1. **Restore** stock PNGs from `backup/icons/` (if a backup exists).
2. **Backup** current stock PNGs with *first snapshot wins* (`ignore-existing`).
3. **SFOS pack** (optional): copy pack `jolla/` PNGs into silica `z/icons/`, and `native/` into hicolor (`existing-only`).
4. **SFOS overlay** (optional): composite pack `overlay/` onto hicolor apps not covered by the pack.
5. **APK last** (when pack and/or overlay selected): copy pack `apk/` and/or overlay composites into `apkd-bridge/launcherIcon/` (`existing-only` for pack icons).
6. **Touch** launcher `.desktop` files (`futimens`; optional lipstick restart when APK PNGs were written and **Restart homescreen** is on).

**Native `.desktop` files are never modified** — `Icon=` stays as shipped; only hicolor PNGs behind that name change.

**Android:** pack and overlay write themed PNGs into `launcherIcon/` when stock already exists there. `apkd_launcher_*.desktop` `Icon=` lines are not modified. Restore brings back stock PNGs from backup and removes any leftover `custom/` directory from earlier betas.

**Silica** (`/usr/share/themes/sailfish-default/silica/<z>/icons/`) is updated for pack `jolla/` icons. Other stock paths under `/usr/share/themes/` are not touched.

Active theme is stored in dconf under `/apps/sailfishos-uithemer` (`activeIconPack`, `activeFontPack`, `iconOverlay`, `homeRefresh`, `wizardDone`) after a **successful** apply. Cover sync re-runs the full restore→backup→run→overlay cycle for the active pack.

### Live paths (where themed pixels go)

| Kind | Live path | Pack source |
|------|-----------|-------------|
| Jolla | `/usr/share/themes/sailfish-default/silica/<z>/icons/<icon-key>.png` | `jolla/<z>/icons/` (z cascade) |
| Native | `/usr/share/icons/hicolor/<size>/apps/<icon-key>.png` | `native/<size>/apps/` (size cascade) |
| Android | `/home/defaultuser/.local/share/apkd-bridge/launcherIcon/<key>.png` | `apk/<size>/<key>.png` (pack) or overlay composite |

Theme packs install under `/usr/share/harbour-themepack-<name>/`. Icon PNGs are read from `$HOME/.themepack/<harbour-themepack-name>/` first (classic layout), then from `/usr/share/...` if needed. UI Themer does not modify pack files.

### Stock backup store

```
/usr/share/sailfishos-uithemer/backup/icons/
  jolla/<z>/icons/
  native/<size>/apps/
  apk/
```

Restore copies backup → live only where the live file still exists (`existing-only`).

### Overlay

When enabled, UI Themer composites pack `overlay/*.png` onto stock PNGs in hicolor (native) and `launcherIcon/` (Android). Only icons whose **basename is not already in the pack** receive overlay (`native/` ∪ `jolla/` keys for SFOS; `apk/` keys for Android).

Matching uses **PNG basename** on disk, not `.desktop` `Icon=` fields.

### Launcher refresh

| Kind | Mechanism |
|------|-------------|
| Native / Jolla | `futimens` on `/usr/share/applications/*.desktop`; Lipstick watches hicolor and silica icon paths |
| Android (APK) | `futimens` on `apkd_launcher_*.desktop` after PNG writes |
| Fallback | **Automatic** lipstick restart when **Restart homescreen** (`homeRefresh`) is enabled after apply/restore. **Manual** restart via the Themes or Display density pulley menu (*Restart homescreen*) |

> Bulk **graphic** theming (writing widget PNGs under the old meegotouch tree) was removed in 2.4.2. Pack `jolla/` trees including ambient-style keys remain useful and still apply through the silica path where matching stock files exist.

Concurrent icon, restore, font, or density operations are rejected via `flock` on `/usr/share/sailfishos-uithemer/icon-ops.lock`.

## Author checklist

1. Use the [dummy package](https://github.com/uithemer/harbour-themepack-dummy) as a template.
2. Place launcher and app icons under `native/<size>/apps/`, `jolla/<z>/icons/`, and/or `apk/<size>/` using the **same basename** as the stock PNG (e.g. `icon-launcher-camera.png`).
3. Optional: add `overlay/*.png` (see [Overlays](#overlays) below).
4. Build with `harbour-themepack-*` naming and publish; users need **UI Themer** installed.

## Theme pack layout

Inside `/usr/share/harbour-themepack-<name>/`:

```
native/
  256x256/apps/<icon-key>.png
  172x172/apps/<icon-key>.png
  ...

jolla/
  z2.0/icons/<icon-key>.png
  z1.5/icons/<icon-key>.png
  ...

apk/
  192x192/<launcher_id>.png
  ...

overlay/
  *.png

dyncal/256x256/          (planned — ignored today)
dynclock/256x256/        (planned — ignored today)
```

### Matching (apply and match counts)

A native icon is themed when the pack has `<key>.png` and stock already has `hicolor/*/apps/<key>.png` (`existing-only`).

### Z cascade (jolla apply)

Pack `jolla/<z>/icons/` is copied into live silica `<z>/icons/` using index-aligned cascade (largest z tier in the pack first for each target z). Tiers such as `z1.5-large` are valid destinations.

### APK / apkd

Pack `apk/` PNGs copy into `apkd-bridge/launcherIcon/<key>.png` when stock already exists there (`existing-only`).

## Create your icons

1. Create icons with the image editor of your choice.
2. Place them in `jolla/` (Jolla stock / ambient keys), `native/` (third-party apps), or `apk/` (Android). Example sizes:
   - Native: `native/172x172/apps/`, `native/86x86/apps/`, …
   - Jolla: `jolla/z2.0/icons/` (172×172), `jolla/z1.5/icons/` (129×129), `jolla/z1.0/icons/` (86×86)
   - Android: `apk/192x192/`, `apk/128x128/`, `apk/86x86/`

Stock reference paths on device (read-only):

* `/usr/share/themes/sailfish-default/silica/<z>/icons/` — Jolla / silica icons (apply target for pack `jolla/`)
* `/usr/share/icons/hicolor` — native app icons
* `/home/defaultuser/.local/share/apkd-bridge/launcherIcon/` — Android launcher icons

### Jolla Ambient

Jolla Ambient is the set of stock icons used across native apps (controls, status bar, covers, camera, and launchers). In a theme pack, ship them under **`jolla/<z>/icons/<icon-key>.png`**. UI Themer copies matching keys into live silica (`existing-only`), the same path used for launcher icons.

Ambient artwork is still widely used in packs (prefixes such as `graphic-*`, `icon-status-*`, `icon-launcher-*`, etc.). Bulk replacement of the entire meegotouch widget tree is no longer supported, but many ambient keys **do** theme when the stock file exists under silica.

Icon families (by filename prefix):

* *graphic-* — general UI and stock app graphics
* *graphic-service-*, *icon-m-service-*, *icon-s-service-* — account settings
* *icon-camera-* — camera app
* *icon-cover-* — cover actions
* *icon-direction-* — maps
* *icon-l-*, *icon-m-*, *icon-s-* — general-purpose UI icons
* *icon-launcher-* — app and folder launchers
* *icon-lock-* — lock screen notification
* *icon-lock-emergency-call* / *icon-lockscreen-emergency-call* — emergency call
* *icon-status-* / *icon-system-* — status bar

#### z1.0

Pack paths: `jolla/z1.0/icons/`. Typical sizes:

* *graphic-* — various dimensions
* *graphic-service-*: 135×135px; *icon-m-service-* 64×64px; *icon-s-service-* 32×32px
* *icon-camera-*: mostly 48×48px (shutter 64×64px)
* *icon-cover-*: 32×32px
* *icon-direction-*: 128×128px
* *icon-l-*: 96×96px; *icon-m-* 64×64px (*icon-m-incoming-call* / *icon-m-missed-call* 42×42px); *icon-s-* 32×32px
* *icon-launcher-*: 86×86px
* *icon-lock-*: 32×32px
* *icon-lock-emergency-call* / *icon-lockscreen-emergency-call*: 64×64px
* *icon-status-* / *icon-system-*: 24×24px

#### z1.5

Pack paths: `jolla/z1.5/icons/`. Sizes are roughly 1.5× the z1.0 list (e.g. *icon-launcher-* 129×129px, *icon-status-* 36×36px).

#### z2.0

Pack paths: `jolla/z2.0/icons/`. Sizes are roughly 2× the z1.0 list (e.g. *icon-launcher-* 172×172px, *icon-status-* 48×48px).

#### References

* [Sailfish documentation — Jolla Ambient](https://sailfishos.org/develop/docs/jolla-ambient/)

### DynCal

{: .note }
**Not applied by UI Themer 3.x yet.** Pack `dyncal/` is ignored by the current engine (support was removed in 2.4.0 and is planned to return). You may still ship this layout so packs are ready later.

[DynCal](https://github.com/fravaccaro/harbour-dyncal) skinning: place icons in `dyncal/256x256/`:

* `dd.png` — day of month (`01`–`31`)
* `mmdd.png` — holiday icons (month + day)

When support returns, UI Themer will apply these only if DynCal is installed.

### DynClock

{: .note }
**Not applied by UI Themer 3.x yet.** Pack `dynclock/` is ignored by the current engine (planned to return).

[DynClock](https://github.com/fravaccaro/harbour-dynclock) skinning:

1. Download `bg.png`, `hour.png`, and `minute.png` from the [DynClock package tree](https://github.com/fravaccaro/harbour-dynclock/tree/master/harbour-dynclock/usr/share/harbour-dynclock).
2. Edit them as you like.
3. Place them in `dynclock/256x256/`.

## Overlays

If your theme uses a consistent mask or frame, add PNGs under `overlay/`. UI Themer picks a random overlay base and composites it onto stock icons **not** already covered by the pack. Use a canvas sized for the target (recommended **192×192** or **172×172** for app icons).

The old Android-only overlay trick (root file `type` containing `android`) is **no longer supported**.

## Currently ignored pack contents

| Path / file | Status |
|-------------|--------|
| `sound/` | Removed in 2.4.4 — see [Sounds](sounds) |
| `dyncal/`, `dynclock/` | Documented, planned — see above |
| Root `type` (`android` overlay-only packs) | Dropped in 2.7.1 |

## Restore

**Restore theme** (pulley menu) restores stock PNGs from the backup store, removes leftover `custom/` under apkd-bridge, clears the backup tree, touches launchers, and resets `activeIconPack` after success.

## Icon file size hints

| Asset | Recommended size |
| ----- | ---------------- |
| Native app | 172×172 (preferred), down to 86×86 |
| Jolla / ambient | per silica z tier in the pack |
| APK app | 192×192 (preferred), down to 86×86 |
| Overlay | 192×192 / 172×172 composite canvas |
