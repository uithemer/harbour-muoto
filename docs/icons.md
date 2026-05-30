---
layout: default
title: Icon pack guidelines
parent: Create theme packs
nav_order: 1
---

# Icon pack guidelines

How to create icons compatible with **Muoto**.

## Author checklist

1. Use the [example package](https://github.com/uithemer/harbour-themepack-example) as a template.
2. Place launcher and app icons under `native/<size>/apps/`, `jolla/<z>/icons/`, and/or `apk/<size>/` using the **same basename** as the stock PNG (e.g. `icon-launcher-camera.png`).
3. Optional: add `overlay/*.png` (see [Style missing app icons (`overlay/`)](#style-missing-app-icons-overlay) below).
4. Build with `harbour-themepack-*` naming and publish; users need **Muoto** installed.

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

### Matching

Icons are matched by **PNG basename** (the filename without `.png`). A stock icon is themed only when a matching PNG already exists on the device (`existing-only`) — name your pack files after the stock icon keys.

Provide `jolla/` tiers (`z1.0`, `z1.5`, `z2.0`, …) as needed; Muoto uses the best available size for each target.

## Create your icons

1. Create icons with the image editor of your choice.
2. Place them in `jolla/` (Jolla stock / ambient keys), `native/` (third-party apps), or `apk/` (Android). Example sizes:
   - Native: `native/172x172/apps/`, `native/86x86/apps/`, …
   - Jolla: `jolla/z2.0/icons/` (172×172), `jolla/z1.5/icons/` (129×129), `jolla/z1.0/icons/` (86×86)
   - Android: `apk/192x192/`, `apk/128x128/`, `apk/86x86/`

Stock reference paths on device (read-only):

* `/usr/share/themes/sailfish-default/silica/<z>/icons/` — Jolla / silica icons
* `/usr/share/icons/hicolor` — native app icons
* `/home/defaultuser/.local/share/apkd-bridge/launcherIcon/` — Android launcher icons

### Jolla Ambient

Jolla Ambient is the set of stock icons used across native apps (controls, status bar, covers, camera, and launchers). In a theme pack, ship them under **`jolla/<z>/icons/<icon-key>.png`**.

Ambient artwork is still widely used in packs (prefixes such as `graphic-*`, `icon-status-*`, `icon-launcher-*`, etc.). Ship matching keys under `jolla/<z>/icons/`; Muoto applies them under `/usr/share/themes/sailfish-default/silica/<z>/icons/` where stock files already exist.

Icon families (by filename prefix):

* *graphic-* — general UI and stock app graphics
* *graphic-service-*, *icon-m-service-*, *icon-s-service-* — account settings
* *icon-camera-* — camera app
* *icon-cover-* — cover actions
* *icon-direction-* — maps
* *icon-l-*, *icon-m-*, *icon-s-* — general-purpose UI icons
* *icon-launcher-* — app and folder launchers
* *icon-lock-* — lock screen notification
* *icon-lock-emergency-call* / *icon-lockscreen-emergency-call*: emergency call
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
**Not applied by Muoto 3.x yet.** Pack `dyncal/` is ignored by the current engine (support was removed in 2.4.0 and is planned to return). You may still ship this layout so packs are ready later.

[DynCal](https://github.com/fravaccaro/harbour-dyncal) skinning: place icons in `dyncal/256x256/`:

* `dd.png` — day of month (`01`–`31`)
* `mmdd.png` — holiday icons (month + day)

When support returns, Muoto will apply these only if DynCal is installed.

### DynClock

{: .note }
**Not applied by Muoto 3.x yet.** Pack `dynclock/` is ignored by the current engine (planned to return).

[DynClock](https://github.com/fravaccaro/harbour-dynclock) skinning:

1. Download `bg.png`, `hour.png`, and `minute.png` from the [DynClock package tree](https://github.com/fravaccaro/harbour-dynclock/tree/master/harbour-dynclock/usr/share/harbour-dynclock).
2. Edit them as you like.
3. Place them in `dynclock/256x256/`.

## Style missing app icons (`overlay/`)

If your theme uses a consistent mask or frame, add PNGs under `overlay/`. Muoto composites them onto stock icons **not** already covered by the pack. Use a canvas sized for the target (recommended **192×192** or **172×172** for app icons).

The old Android-only overlay trick (root file `type` containing `android`) is **no longer supported**.

## Currently ignored pack contents

| Path / file | Status |
|-------------|--------|
| `sound/` | Removed in 2.4.4 — see [Sounds](sounds) |
| `dyncal/`, `dynclock/` | Documented, planned — see above |
| Root `type` (`android` overlay-only packs) | Dropped in 2.7.1 |

## Icon file size hints

| Asset | Recommended size |
| ----- | ---------------- |
| Native app | 172×172 (preferred), down to 86×86 |
| Jolla / ambient | per silica z tier in the pack |
| APK app | 192×192 (preferred), down to 86×86 |
| Missing-icon frame | 192×192 / 172×172 composite canvas |
