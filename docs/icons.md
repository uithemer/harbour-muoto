---
layout: default
title: Icon pack guidelines
parent: Get started
nav_order: 1
---

# Icon pack guidelines

How to create icons compatible with UI Themer.

## How icons are applied

Starting from UI Themer 2.4, icons are applied by **rewriting the `Icon=` entry
inside `.desktop` files** (no PNG copies). UI Themer scans:

* `/usr/share/applications/*.desktop` for native apps.
* `/home/defaultuser/.local/share/applications/apkd_launcher_*.desktop` for
  Android launcher entries.

For each `.desktop` whose `Icon=` value matches a PNG inside the theme pack,
UI Themer:

1. Records the original `Icon=` value in
   `/usr/share/sailfishos-uithemer/icon-backup.json`.
2. Replaces the `Icon=` line with the absolute path of the matching PNG inside
   the theme pack (no copies, no overwrites of system files).

Native and APK lookups are both keyed off the `.desktop`'s `Icon=` value (not
its filename). For native apps this is what makes built-in Jolla launchers
match the pack: e.g. `jolla-camera.desktop` ships `Icon=icon-launcher-camera`,
so the pack only needs to provide `icon-launcher-camera.png` (no
`jolla-camera.png` required).

A boot-time oneshot service (`sailfishos-uithemer-icond.service`)
re-asserts the active theme, so package updates that overwrite a `.desktop`
file are transparently re-themed at the next boot. A pre-system-update service
(`themepacksupport-systemupgrade.service`) restores all `Icon=` entries to
their originals before the upgrade runs, so RPM never sees modified files.

> Graphic theming for Sailfish system widgets (the old PNG-replacement
> pipeline that copied `<pack>/graphic/*` into
> `/usr/share/themes/sailfish-default/meegotouch/icons/`) has been
> **removed** in 2.4.2. The `dyncal/` and `dynclock/` directories shipped
> by older themes are **ignored**: they are still allowed inside the theme
> pack but no longer applied.
>
> Since 2.4.3, the pre-3.0 Jolla ambient icon subtree
> (`<pack>/jolla/zX.Y/icons/`) is consulted again, but **only** as a
> fallback source of PNGs that the launcher will reference via `Icon=` —
> nothing under `jolla/` is ever copied into
> `/usr/share/themes/sailfish-default/meegotouch/`.

## Theme pack layout

Inside `/usr/share/harbour-themepack-<name>/` UI Themer looks for:

```
native/
  256x256/apps/<icon-key>.png
  172x172/apps/<icon-key>.png
  128x128/apps/<icon-key>.png
  108x108/apps/<icon-key>.png
   86x86/apps/<icon-key>.png

jolla/                            # legacy fallback, see "Lookup order" below
  z2.0/icons/<icon-key>.png
  z1.75/icons/<icon-key>.png
  z1.5-large/icons/<icon-key>.png
  z1.5/icons/<icon-key>.png
  z1.25/icons/<icon-key>.png
  z1.0/icons/<icon-key>.png

apk/
  192x192/<launcher_id>.png
  128x128/<launcher_id>.png
   86x86/<launcher_id>.png

overlay/
  *.png    # one or more 512x512 overlay base images (optional)
```

Where:

* `<icon-key>` is the literal value of `Icon=` inside the system `.desktop`
  file (e.g. `harbour-storeman.desktop` ships `Icon=harbour-storeman`, so the
  pack provides `harbour-storeman.png`; `jolla-camera.desktop` ships
  `Icon=icon-launcher-camera`, so the pack provides
  `icon-launcher-camera.png`).
* `<launcher_id>` is the literal value of `Icon=` inside an APK
  `apkd_launcher_*.desktop` file (e.g.
  `apkd_launcher_org.example.bar.png`).

### Lookup order (native)

For each native `.desktop`, UI Themer reads the current `Icon=` value,
normalises it to a bare key (strips any leading directory and trailing
`.png`), and tries, in order:

1. `<pack>/native/<size>/apps/<icon-key>.png` for `<size>` in
   `256x256, 172x172, 128x128, 108x108, 86x86` (largest first).
2. `<pack>/jolla/<zSize>/icons/<icon-key>.png` for `<zSize>` in
   `z2.0, z1.75, z1.5-large, z1.5, z1.25, z1.0` (largest first).

First hit wins. The `jolla/` step is a **lookup-only** fallback for older
packs: matched PNGs are referenced from the launcher via `Icon=`, never
copied into the system theme tree.

For APK icons UI Themer picks the **largest** size present in `apk/` and
points `Icon=` at that absolute path. Prefer **`apk/192x192/`**, since modern
apkd-bridge does not downscale to smaller buckets; `128x128`/`86x86` remain
only as fallbacks when `192x192` is missing.

## Overlays

If your theme ships an `overlay/` folder, UI Themer can use it to fill in apps
that do not have a dedicated icon in the pack. When the user enables
"Apply icon overlay" in the confirm dialog, UI Themer:

1. Picks one random `*.png` from `overlay/` as the base.
2. Scales the original app icon down (122x122 for APK on a 192x192 canvas;
   60% of the size for native).
3. Composites them with QPainter and stores the result under
   `~/.cache/sailfishos-uithemer/overlay/<pack>/`.
4. Points `Icon=` at that cached PNG.

Overlay PNGs should be sized 192x192 for best APK results; native uses up to
172x172.

### Android-only overlays

If your theme contains **only** overlays (no `apk/` or `native/`), drop a
file named `type` at the root of the theme pack containing the single line
`android` to indicate it should only target APK apps. Without this file
overlay also applies to native apps.

## Restore

`Restore theme` (or the helper `sailfishos-uithemer-icond --restore`)
walks `icon-backup.json` and writes each entry's `original_icon` back into
its `.desktop` file, then drops the entry. Apps whose `.desktop` was deleted
in the meantime are silently dropped from the manifest.

## Icon file size hints

For reference, common SailfishOS sizes:

| Asset       | Recommended size  |
| ----------- | ----------------- |
| Native app  | 172x172 (preferred), down to 86x86 |
| APK app     | 192x192 (preferred), down to 86x86 |
| Overlay     | 192x192 base canvas |
