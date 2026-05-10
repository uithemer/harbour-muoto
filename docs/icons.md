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

For each `.desktop` whose name (or, for APK entries, whose `Icon=` value) matches
a PNG inside the theme pack, UI Themer:

1. Records the original `Icon=` value in
   `/usr/share/sailfishos-uithemer/icon-backup.json`.
2. Replaces the `Icon=` line with the absolute path of the matching PNG inside
   the theme pack (no copies, no overwrites of system files).

A boot-time oneshot service (`sailfishos-uithemer-reassert.service`)
re-asserts the active theme, so package updates that overwrite a `.desktop`
file are transparently re-themed at the next boot. A pre-system-update service
(`themepacksupport-systemupgrade.service`) restores all `Icon=` entries to
their originals before the upgrade runs, so RPM never sees modified files.

> Jolla ambient icons (`jolla/z1.0`, `z1.5`, `z2.0` ...), `dyncal/` and
> `dynclock/` directories shipped by older themes are **ignored**: they are
> still allowed inside the theme pack but no longer applied. Themes can drop
> them safely.

## Theme pack layout

Inside `/usr/share/harbour-themepack-<name>/` UI Themer looks for:

```
native/
  256x256/apps/<base>.png
  172x172/apps/<base>.png
  128x128/apps/<base>.png
  108x108/apps/<base>.png
   86x86/apps/<base>.png

apk/
  192x192/<launcher_id>.png
  128x128/<launcher_id>.png
   86x86/<launcher_id>.png

overlay/
  *.png    # one or more 512x512 overlay base images (optional)
```

Where:

* `<base>` is the basename of a system `.desktop` file (e.g.
  `harbour-storeman.desktop` -> `harbour-storeman.png`).
* `<launcher_id>` is the literal value of `Icon=` inside an APK
  `apkd_launcher_*.desktop` file (e.g.
  `apkd_launcher_org.example.bar.png`).

For each matched app UI Themer picks the **largest** size present and points
`Icon=` at that absolute path. Smaller buckets are still useful to support
displays that pick a smaller icon natively.

For APK icons: prefer **`apk/192x192/`**, since modern apkd-bridge does not
downscale to smaller buckets; `128x128`/`86x86` remain only as fallbacks when
`192x192` is missing.

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

`Restore theme` (or the helper `sailfishos-uithemer-reassert --restore`)
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
