---
layout: default
title: Architecture
parent: Developers
nav_order: 1
---

# Architecture

How Muoto theming works at runtime (3.2+). Pack-author layout is documented under [Icon pack guidelines](../icons); this page is the engine.

## Processes

```mermaid
flowchart LR
  GUI[harbour-muoto GUI]
  IC[launcher-icond session]
  HD[helperd system]
  IL[install-listener]
  UI[update-icons / oneshot-restore]
  L[Lipstick]

  GUI -->|session D-Bus ApplyIcons| IC
  GUI -->|system D-Bus density uninstall| HD
  IL -->|exec update-icons| UI
  UI -->|session ApplyIcons / RestoreIcons| IC
  IC -->|inplace hicolor / redirect APK| L
```

| Process | Bus / unit | Role |
| ------- | ---------- | ---- |
| `harbour-muoto` | user GUI | Mosaic home, configure pages, density; calls Helper / Launcher |
| `harbour-muoto-launcher-icond` | session `org.muoto.Launcher1` | All launcher icon apply/restore; `cap_dac_override` for system `.desktop` / silica folder writeback |
| `harbour-muoto-helperd` | system `org.muoto.Muoto1` | On-demand: `DensityEnable`, `UninstallPack` only |
| `harbour-muoto-install-listener` | user unit | D-Bus hooks → `/usr/bin/harbour-muoto-update-icons` |
| Boot / upgrade scripts | root systemd oneshots | Re-apply or pre-upgrade restore — see [Automation](automation) |

## Icon apply (launcher daemon)

1. GUI (or `update-icons`) sets dconf `activeIconPack` / `iconOverlay`, then calls session `org.muoto.Launcher1.Themes.ApplyIcons`.
2. `LauncherIconOps` re-arms Lipstick's watches on the APK desktops (see below), rebuilds one `IconUpdater` per launcher `.desktop` (system + user APK desktops), then applies homescreen **folders** via `FolderAmbient`.
3. Pack assets come from `/usr/share/harbour-themepack-<name>/` (`jolla/`, `native/`, `apk/` — often symlinked to `~/.themepack/…`). Overlay frames from `overlay/` composite onto stock when the pack has no matching icon.
4. **Write model (hybrid):**
   - **Hicolor** (native / many overlay targets): **inplace** — keep `Icon=` as the theme name; replace the single resolved launcher-size PNG under `/usr/share/icons/hicolor/<N>x<N>/apps/` (`N` ≥ `iconSizeLauncher`, first hit); `futimens` the `.desktop`. Other hicolor sizes stay stock.
   - **APK bridge**: **redirect** — write `/usr/share/harbour-muoto/launcher-icons/<desktop>-<msecs>.png`, set `Icon=` to that path, touch the desktop (absolute paths need a new `Icon=` for Lipstick to refresh).
5. Original `Icon=` / paths are tracked in dconf `saved-id`, fingerprints (inplace), and `launcher-manifest.json`.
6. Homescreen **folders** (`icon-launcher-folder-01`…`16`) use scoped silica writeback + `backup/folder-icons/` (`FolderAmbient`).
7. Dynamic clock/calendar use pack `dynclock/` / `dyncal/` (or stock SVG when pack is `default`) when dconf dyn flags are on.

Restore uses the manifest (redirect + inplace backups), restores folder backups, clears generated `launcher-icons/`, and sets `activeIconPack` to `default`.

### Re-arming Lipstick's desktop watches

Lipstick's `LauncherMonitor` holds a per-file inotify watch on every `.desktop` it has discovered, and `LauncherModel` only re-reads an entry when that watch fires. apkd regenerates `apkd_launcher_*.desktop` with `rename(2)` whenever the Android container is rebuilt; Qt drops the watch on the replaced inode, and `onDirectoryChanged` only calls `addPaths()` for filenames it has not seen before — so the watch is gone for good. Measured on device, all 15 APK desktops were unwatched while all 76 system ones were fine. From then on the `Icon=` redirect is invisible and the tiles need a homescreen restart.

`LauncherWatch::rearmDesktopWatches` (`src/launcher/launcherwatch.cpp`) fixes this by renaming each entry to `<name>.muoto-rearm` and back, batched across all APK desktops:

- The two renames land in **separate** directory scans (400 ms apart), so Lipstick actually observes the name leaving and returning and calls `addPaths()` again.
- Both scans fall inside the 2000 ms `LAUNCHER_MONITOR_HOLDBACK_TIMEOUT_MS` window, so the pending remove and add cancel out and no launcher item is rebuilt — grid positions in `[LauncherOrder]` are untouched.
- `rename(2)` keeps the inode, owner, mode and contents, so nothing else about the entry changes.
- Callers then wait out the remaining holdback before rewriting `Icon=`. The waits use a nested `QEventLoop` so the daemon keeps serving D-Bus.

`applyIcons()` and `restoreIcons()` both call it up front — restore rewrites `Icon=` too and needs a live watch just as much. `rebuildIconUpdatersNow()` sweeps leftover `*.muoto-rearm` files, which also covers a daemon killed mid-round-trip since the daemon rebuilds at startup.

A homescreen restart is still needed after Android support itself restarts: apkd regenerates the desktops a while after `containerReady`, and the install-listener's re-apply does not currently retrigger on that.

**Not themed in 3.2+:** bulk silica ambient (`graphic-*`, status bar, etc.). Only launcher-relevant `icon-launcher-*` / native / APK paths.

## dconf (`/apps/harbour-muoto/`)

| Key | Purpose |
| --- | ------- |
| `activeIconPack` | Short pack name or `default` |
| `iconOverlay` | Style missing icons |
| `homeRefresh` | Restart Lipstick after apply (GUI) |
| `launcher/dynamicClockEnabled` | Live clock icon |
| `launcher/dynamicCalendarEnabled` | Live calendar icon |
| `launcher/applications/<desktop>/provider` | Only `dynamic-icon://…` is honored (clock/calendar) |
| `launcher/saved-id/<desktop>` | Original `Icon=` before redirect (and related restore state) |
| `launcher/fingerprint/<hash>` | Inplace “our bytes” marker for hicolor paths |

## Source map

| Area | Path |
| ---- | ---- |
| Apply / rebuild | `src/launcher/launchericonops.cpp` |
| Per-desktop update | `src/launcher/iconupdater.cpp`, `desktopentry.cpp` |
| Pack index | `src/launcher/harbourthemepack.cpp` |
| Path resolve (hicolor / APK) | `src/launcher/iconresolve.cpp` |
| Lipstick watch re-arm | `src/launcher/launcherwatch.cpp` |
| Overlay | `src/launcher/overlayiconprovider.cpp`, `overlayrender.cpp` |
| Folder silica | `src/launcher/folderambient.cpp` |
| Manifest | `src/launcher/launchermanifest.cpp` |
| Daemon main | `src/launcher-daemon/main.cpp` |
| Font apply / restore | `src/gui/fontapplier.cpp` (stages under `~/.local/share/fonts/muoto/`) |
| Mosaic home / theme work | `qml/pages/MainPage.qml`, `qml/components/ThemeWork.qml` |
| Icons / fonts configure | `qml/pages/IconsConfigurePage.qml`, `qml/pages/FontsConfigurePage.qml` |
| Dyn icons | `qml/pages/DynamicIconsPage.qml` |
| Session D-Bus XML | `dbus/org.muoto.Launcher1.Themes.xml` |
| Shell helpers | `service/muoto-dbus-wait.sh`, `service/harbour-muoto-update-icons` |
