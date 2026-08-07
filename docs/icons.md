---
layout: default
title: Icon pack guidelines
parent: Create theme packs
nav_order: 1
---

# Icon pack guidelines

How to create icons compatible with **Muoto**.

## Author checklist

1. Copy the [`icon-theme/`](https://github.com/uithemer/harbour-themepack-example/tree/master/icon-theme) or [`full-theme/`](https://github.com/uithemer/harbour-themepack-example/tree/master/full-theme) template from the example repo.
2. Place launcher and app icons under `theme/native/<size>/apps/`, `theme/jolla/<z>/icons/`, and/or `theme/apk/<size>/` using the **same basename** as the stock PNG (e.g. `icon-launcher-camera.png`).
3. Optional: add `theme/overlay/*.png` (see [Style missing app icons (`overlay/`)](#style-missing-app-icons-overlay) below). Export SVG sources with [themepack-helper](https://github.com/uithemer/harbour-themepack-example/tree/master/themehelper).
4. Build with the Sailfish SDK — see [Building](getstarted#building) in the getstarted guide. Publish on OpenRepos; users need **Muoto** installed.

## Theme pack layout

When authoring a pack, place assets under `theme/` in your project (see [Project layout](getstarted#project-layout)). After installation on a device, Muoto reads them from `/usr/share/harbour-themepack-<name>/`:

```
theme/                         →  /usr/share/harbour-themepack-<name>/
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

  dyncal/256x256/          (optional — dynamic calendar)
  dynclock/256x256/        (optional — dynamic clock)
```

### Matching

Icons are matched by **PNG basename** (the filename without `.png`). A stock icon is themed only when a matching PNG already exists on the device (`existing-only`) — name your pack files after the stock icon keys.

Provide `jolla/` tiers (`z1.0`, `z1.5`, `z2.0`, …) as needed; Muoto uses the best available size for each target.

## Create your icons

1. Create icons with the image editor of your choice.
2. Place them in `theme/jolla/` (Jolla stock / ambient keys), `theme/native/` (third-party apps), or `theme/apk/` (Android). Example sizes:
   - Native: `theme/native/172x172/apps/`, `theme/native/86x86/apps/`, …
   - Jolla: `theme/jolla/z2.0/icons/` (172×172), `theme/jolla/z1.5/icons/` (129×129), `theme/jolla/z1.0/icons/` (86×86)
   - Android: `theme/apk/192x192/`, `theme/apk/128x128/`, `theme/apk/86x86/`

Stock reference paths on device (read-only):

* `/usr/share/themes/sailfish-default/silica/<z>/icons/` — Jolla / silica icons
* `/usr/share/icons/hicolor` — native app icons
* `/home/defaultuser/.local/share/apkd-bridge/launcherIcon/` — Android launcher icons

### Jolla Ambient (launcher icons only in 3.2+)

From **Muoto 3.2**, only **`icon-launcher-*`** keys under `jolla/<z>/icons/` are themed (via the launcher daemon). Status bar, covers, in-app `graphic-*`, and other ambient families are **not** bulk-copied into silica anymore.

Ship launcher keys under `jolla/<z>/icons/`; the daemon redirects `.desktop` `Icon=` to generated PNGs under `/usr/share/harbour-muoto/launcher-icons/`. Engine details: [Architecture](devel/architecture).

Homescreen **folders** use theme ids `icon-launcher-folder-01` … `16` (`image://theme/` / Lipstick `Folder*.directory`). Muoto themes those by scoped writeback into sailfish-default silica (`icon-launcher-folder-NN.png` only), with backups under `/usr/share/harbour-muoto/backup/folder-icons/`. Ship matching PNGs under `jolla/`, or enable overlay to replace stock folder glyphs with the overlay frame alone (no inner stock icon). Folder **picker** and tiles then both show the themed silica assets.

### DynCal

Day-of-month Calendar launcher icon from the launcher daemon. With a theme pack that ships `dyncal/`, day icons come from `dyncal/256x256/{01..31}.png`.

Pack layout:

* `dd.png` — day of month (`01`–`31`)
* `mmdd.png` — holiday icons (month + day, optional)

Enable on **Confirm** when applying a pack that includes `dyncal/`, or on the **Dynamic icons** tab while such a pack is active (or on default after restore — UI available, switches off until enabled). The tab is greyed out only when a non-default pack without dyn assets is active.

### DynClock

Live Clock launcher icon. With a theme pack that ships `dynclock/`, assets under `dynclock/256x256/` (`bg.png`, `hour.png`, `minute.png`) drive the live hands.

Enable on **Confirm** when applying a pack that includes `dynclock/`, or on the **Dynamic icons** tab while such a pack is active (or on default after restore — UI available, switches off until enabled). The tab is greyed out only when a non-default pack without dyn assets is active.

## Style missing app icons (`overlay/`)

If your theme uses a consistent mask or frame, add PNGs under `overlay/`. Muoto composites them onto **launcher-visible** stock icons not already covered by the pack, via generated PNGs and `.desktop` redirect — stock files under hicolor and `apkd-bridge/launcherIcon/` are **not** modified.

The old Android-only overlay trick (root file `type` containing `android`) is **no longer supported**.

## Currently ignored pack contents

| Path / file | Status |
|-------------|--------|
| `sound/` | Removed in 2.4.4 — see [Sounds](sounds) |
| `dyncal/`, `dynclock/` | Launcher daemon when enabled via Confirm / Dynamic icons tab; pack assets when present |
| Root `type` (`android` overlay-only packs) | Dropped in 2.7.1 |

## Icon file size hints

| Asset | Recommended size |
| ----- | ---------------- |
| Native app | 172×172 (preferred), down to 86×86 |
| Jolla / ambient | per silica z tier in the pack |
| APK app | 192×192 (preferred), down to 86×86 |
| Missing-icon frame | 192×192 / 172×172 composite canvas |
