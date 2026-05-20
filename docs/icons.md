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
inside `.desktop` files**. UI Themer scans:

* `/usr/share/applications/*.desktop` for native apps.
* `/home/defaultuser/.local/share/applications/apkd_launcher_*.desktop` for
  Android launcher entries.

For each `.desktop` that matches a theme pack (or overlay, when enabled), UI
Themer:

1. Records the original `Icon=` value in
   `/usr/share/sailfishos-uithemer/icon-backup.json`.
2. Publishes PNGs under
   `/usr/share/icons/hicolor/<size>/apps/<pack-short>/`.
3. Sets `Icon=<pack-short>/<icon-key>` on the `.desktop` (e.g.
   `Icon=numix-circle/harbour-talteen`).

`<pack-short>` is the pack name without the `harbour-themepack-` prefix
(e.g. `numix-circle` for `harbour-themepack-numix-circle`).

### Hicolor publish tree

All themed pixels for a pack live in one place per launcher size, for example:

```text
/usr/share/icons/hicolor/108x108/apps/numix-circle/
  harbour-talteen.png
  icon-launcher-camera.png
  apkd_launcher_org_fdroid_fdroid.png
```

The same keys exist under `86x86`, `128x128`, `172x172`, and `256x256`.

Sources copied or generated into that tree:

| Source | Publish |
|--------|---------|
| `native/<size>/apps/` | Copy into matching `hicolor/<size>/apps/<pack-short>/` |
| `jolla/<zSize>/icons/` | For keys missing from native: scale largest `z*` asset into all hicolor sizes |
| `apk/<size>/` | Copy per size when present; otherwise scale from largest APK asset |
| Overlay (optional) | Composite in memory, write into the same hicolor paths |

Theme packs under `/usr/share/harbour-themepack-<name>/` are **read-only**
sources; UI Themer does not modify or symlink them.

Native, APK, and apkd launchers all use the same `Icon=<pack-short>/<key>` form.
UI Themer does **not** copy icons into `apkd-bridge/launcherIcon/`.

### How the launcher refreshes

After UI Themer writes a new `Icon=` and saves the `.desktop`, Lipstick's
`QFileSystemWatcher` reloads that launcher entry within about a second. A full
`systemctl --user restart lipstick.service` is **not** required for normal apply
or restore.

To theme apps installed or updated after the last apply, use the **cover sync**
action: it re-runs `ApplyIcons` for the active pack using the overlay flag saved
in dconf at apply time (`settings.iconOverlay`).

> Graphic theming for Sailfish system widgets (the old PNG-replacement
> pipeline) has been **removed** in 2.4.2.
>
> Since 2.4.3, the pre-3.0 Jolla ambient icon subtree
> (`<pack>/jolla/zX.Y/icons/`) is used as a fallback when publishing into
> hicolor.

## Theme pack layout

Inside `/usr/share/harbour-themepack-<name>/` UI Themer looks for:

```
native/
  256x256/apps/<icon-key>.png
  172x172/apps/<icon-key>.png
  128x128/apps/<icon-key>.png
  108x108/apps/<icon-key>.png
   86x86/apps/<icon-key>.png

jolla/                            # legacy fallback
  z2.0/icons/<icon-key>.png
  ...

apk/
  192x192/<launcher_id>.png
  128x128/<launcher_id>.png
   86x86/<launcher_id>.png

overlay/
  *.png    # optional overlay base images
```

### Lookup order (native)

For each native `.desktop`, UI Themer normalises `Icon=` to a bare key and
tries:

1. `<pack>/native/<size>/apps/<icon-key>.png` (largest size first).
2. `<pack>/jolla/<zSize>/icons/<icon-key>.png` (largest `z*` first).

### APK / apkd

UI Themer uses the largest PNG under `<pack>/apk/` and publishes it into
`hicolor/.../<pack-short>/`. The `.desktop` gets
`Icon=<pack-short>/apkd_launcher_<id>`.

## Overlays

When the user enables **Apply icon overlay**, UI Themer composites a random
`overlay/*.png` base with the stock app icon for apps that have **no** matching
pack PNG, then writes the result directly into
`hicolor/<size>/apps/<pack-short>/` (no separate overlay cache directory).

## Restore

`Restore theme` walks `icon-backup.json`, restores each `original_icon` on
its `.desktop` file, deletes
`/usr/share/icons/hicolor/*/apps/<pack-short>/`, and clears the manifest.

## Icon file size hints

| Asset       | Recommended size  |
| ----------- | ----------------- |
| Native app  | 172x172 (preferred), down to 86x86 |
| APK app     | 192x192 (preferred), down to 86x86 |
| Overlay     | 192x192 / 172x172 composite canvas |
