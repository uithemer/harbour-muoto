# AGENTS.md

Orientation for coding agents working on **harbour-muoto** (Sailfish OS theming app).

## What this is

Muoto lets users apply `harbour-themepack-*` icon/font packs and display density on Sailfish OS. From **3.2**, launcher icons are applied only by the session daemon `harbour-muoto-launcher-icond`. The root helperd is slim (density unlock + uninstall pack). Fonts apply in-process in the GUI as `defaultuser`.

## Docs map

| Audience | Where |
| -------- | ----- |
| End users | [docs/guide.md](docs/guide.md), [docs/themes.md](docs/themes.md), [docs/dynamic-icons.md](docs/dynamic-icons.md), [docs/density.md](docs/density.md), [docs/quick-app-switching.md](docs/quick-app-switching.md) |
| Pack authors | [docs/getstarted.md](docs/getstarted.md), [docs/icons.md](docs/icons.md), [docs/fonts.md](docs/fonts.md) |
| Maintainers / agents | [docs/devel/](docs/devel/) — [architecture](docs/devel/architecture.md), [testing](docs/devel/testing.md), [debugging](docs/devel/debugging.md), [automation](docs/devel/automation.md) |

**Before changing icon apply/restore**, read [docs/devel/architecture.md](docs/devel/architecture.md).

## Icons (summary)

- Session D-Bus `org.muoto.Launcher1.Themes` (`ApplyIcons` / `RestoreIcons`).
- **ApplyIcons** sets `activeIconPack` / `iconOverlay`, `rebuildIconUpdaters()`, then `FolderAmbient::apply` (no stock intermediate).
- **Hybrid write model** (`IconUpdater` ctor):
 - **Hicolor** (`/usr/share/icons/hicolor/…/apps/`) → **inplace across every raster size slot**: keep `Icon=harbour-foo`, replace the PNG bytes in each existing `<N>x<N>/apps/` slot (render per slot size), touch desktop. Fingerprint in dconf per slot detects “our” bytes. Never redirect these: Lipstick pins a launcher item to a concrete hicolor path the first time it processes the entry with a bare hicolor-resolvable name (`LauncherItem::m_customIconFilename`), and from then on ignores `Icon=` rewrites entirely — a redirect after any such moment (restore, install, rpm update) is invisible until a homescreen restart. Writing the slot bytes and touching the desktop re-pins with a bumped cache serial, so the tile follows whether pinned or not. `scalable/apps` SVGs are never written.
 - **APK bridge** (`~/.local/share/apkd-bridge/launcherIcon/`) → **redirect**: unique `launcher-icons/<desktop>-<msecs>.png` + `Icon=` rewrite (absolute paths never pin; a new path busts the cache).
 - **Jolla apps** (`icon-launcher-*`) → redirect; those names don’t resolve in hicolor, so they can’t pin.
 - **Overlay** (missing pack icons only): same slot rule — hicolor names get the composite written inplace into every slot (stock loaded from backup), everything else redirects.
- Pack lookup (`HarbourThemePack`): prefer `native|apk/<iconSizeLauncher>x…/`, else largest available pack size; jolla uses `jolla/z<pixelRatio>/`. Pack trees under `/usr/share/harbour-themepack-*` may be **symlinks** into `~/.themepack/` — follow them when debugging “empty native/”.
- Overlay composites pack `overlay/*.png` onto **stock** (backup if live was already themed). Not used when the pack already has that app.
- Manifest holds **one entry per themed slot** for inplace desktops (`LauncherManifest::replaceEntriesForDesktop`); restore puts each slot’s stock backup bytes back and touches the desktop — `Icon=` is never rewritten for hicolor apps, so restore is also restart-free. On a **failed** slot restore the fingerprint is kept, never reset: clearing it made the next apply back up our own bytes as “stock”.
- `saved-id` dconf keeps the original ref for redirect entries; folder tiles via `FolderAmbient` (silica `icon-launcher-folder-*`).
- Per-app dconf `launcher/applications/<desktop>/provider`: only `dynamic-icon://` (clock/calendar) is honored.

## Homescreen icon refresh (do not “just overwrite the PNG”)

Lipstick caches launcher artwork by the desktop `Icon=` string, and **pins** items whose `Icon=` is a bare hicolor-resolvable name to a concrete PNG path, after which `Icon=` rewrites are ignored (see write model above). Muoto’s refresh trick (`IconUpdater`):

1. **Redirect (APK / jolla / non-hicolor)** — write 
 `/usr/share/harbour-muoto/launcher-icons/<desktopBase>-<msecs>.png` 
 (`generateIconPath`), set `Icon=` to that path. A **new path** busts the cache.
2. **Inplace (all hicolor raster slots)** — `FileWrite::inPlace` on each slot (inode preserved); `Icon=` name unchanged; **`futimens` the `.desktop`** so Lipstick re-reads the entry and re-pins from the new bytes.
3. **Always touch the `.desktop`** after PNG / `Icon=` changes (`touchFile` / `futimens`).
4. Avoid deleting a PNG while `Icon=` still names it (`inotify_add_watch` ENOENT → frozen tile until lipstick restart).
5. **Re-arm the watch on APK desktops first.** Lipstick keeps a per-file inotify watch on every `.desktop` and only re-reads one when that watch fires. apkd rewrites `apkd_launcher_*.desktop` with `rename(2)`, so Qt drops the watch, and `LauncherMonitor::onDirectoryChanged` never re-adds it (the filename is already known). After that, `Icon=` rewrites are invisible. `LauncherWatch::rearmDesktopWatches` renames each entry aside and back with a short gap: two directory scans inside Lipstick's 2000 ms holdback, so the pending add and remove cancel — the watch comes back with no launcher item or grid-position churn. Apply and restore both call it before touching `Icon=`.

Implementation: `src/launcher/iconupdater.cpp`, `iconresolve.cpp`, `launcherwatch.cpp`, `overlayiconprovider.cpp`, `folderambient.cpp`.

## Android container restarts

- apkd resets `Icon=` on all `apkd_launcher_*.desktop` whenever the container restarts. `AlienDalvikWatcher` waits for apkd's `containerReady` property (standard `PropertiesChanged` on session path `/com/jolla/apkd`, no sender filter — apkd's bus name changes every restart), then `LauncherIconOps::refreshApkIcons()` re-arms the watches and recreates **only** the APK updaters. No folder pass, no native entries, no `pruneOrphans`.
- apkd syncs the entries a second time a few seconds after `containerReady`, so `refreshApkIcons` schedules a one-shot verification 15 s later and repeats itself if an entry it themed lost our `Icon=`.
- Do not hook per-`IconUpdater` refreshes to container events: a single updater cannot re-arm the watch, so its `Icon=` write goes unread.

## New apps / app updates

Two independent triggers re-theme after an install or update while a pack is active:

- **Listener** (`harbour-muoto-install-listener`) watches PackageKit roles `10` (`InstallFiles`, local RPM), `11` (`InstallPackages`), `22` (`UpdatePackages` — Storeman uses this for both install and update), `33` (`UpgradeSystem`). The `Package` signal is `(u info, s package_id, s summary)`; info `11`/`12` is the fallback. APK installs/updates still use session `com.jolla.apkd` `appInstalled` / `appUpdated`. On a hit it runs `/usr/bin/harbour-muoto-update-icons`, which calls `ApplyIcons`. `activeIconPack` is the full `harbour-themepack-*` name — strip that prefix before building `/usr/share/harbour-themepack-…` or the script no-ops.
- **Daemon watch** (`LauncherIconOps::refreshNewDesktops`): `QFileSystemWatcher` on the applications dirs, 2 s debounce. Attaches an updater for every launcher `.desktop` that has none, and recreates inplace updaters whose hicolor PNG no longer matches the stored fingerprint (an RPM update overwrote it). Native RPM updates `rename(2)` the `.desktop` the same way apkd does, so this path (and `ApplyIcons` / `RestoreIcons`) re-arm Lipstick watches on **all** launcher desktops, not only APK. Same guards as `refreshApkIcons`. This is the path that still works if the listener is down.

## Fonts apply / restore

- Unprivileged, in the **GUI process** via `FontApplier` (`src/gui/fontapplier.cpp`), driven by `ThemePackModel::applyTheme` / `restoreTheme`.
- Icons/Fonts configure pages and `ThemeWork` call these (icons go through `Helper` → launcher-icond in parallel).
- Apply: copy pack `font/` (+ optional `font-nonlatin/`) into `~/.local/share/fonts/muoto/` (Sailjail-readable), write `~/.config/fontconfig/conf.d/99-muoto.conf` with `<dir>` pointing at that staging tree, run `fc-cache`; sets `activeFontPack` in dconf. Real copies — not symlinks into `.themepack`.
- Restore: remove that conf and wipe `~/.local/share/fonts/muoto/`, `fc-cache`, clear `activeFontPack`.
- After upgrading to 3.2.2+, reapply the font once so jailed apps pick up staging (RPM update does not rewrite an existing conf).
- User docs: [docs/fonts.md](docs/fonts.md). UI: `qml/pages/FontsConfigurePage.qml`, `qml/components/ThemeWork.qml`.

## Display density apply / restore

- **Unlock:** Display density dialog calls `Helper.densityEnable()` → system bus `org.muoto.Muoto1` → helperd `DensityEnable` (moves vendor dconf locks so user keys can change). Implemented in `src/ops/densityenabler.cpp` / daemon adaptor.
- **Apply:** On Dialog accept, UI writes user dconf (`desktop/sailfish/silica/theme_pixel_ratio`, launcher icon size keys) once unlocked — see `qml/pages/DensityPage.qml`. Cancel discards pending slider/combo/restore values.
- **Restore:** Per-control Restore default buttons set pending reset; Apply calls `ThemePackModel::restoreDpi` → `DensityEnabler::restoreDensity` (dconf reset of selected keys). Completion is handled on `ThemeWork` (`dpiRestored`).
- User docs: [docs/density.md](docs/density.md).

## Quick app switching

- Surfaces the experimental SFOS gesture (slow ~3 cm edge peek jumps back to the previous app) behind a home tile, so users do not have to write dconf over SSH.
- Plain **user** key `/desktop/sailfish/experimental/quickAppToggleGesture` — no helper, no daemon, no vendor locks to move: the GUI writes it in-process with `ConfigurationValue` and Lipstick applies it live. Nothing to restart.
- Dialog `qml/pages/QuickSwitchPage.qml` uses the pending/applied/dirty pattern (Apply writes the key and toasts, Cancel discards). The `MainPage` tile subtitle binds the same key, so it also tracks changes made outside the app.
- This is a **system** setting Muoto exposes, not theming state: restore, uninstall, and pre-upgrade `oneshot-restore` all leave it alone (that script only touches `/apps/harbour-muoto/*` and `/desktop/sailfish/silica/*`).
- User docs: [docs/quick-app-switching.md](docs/quick-app-switching.md).

## Build and device

- Use the Sailfish SDK (`sfdk`); do not assume a desktop Qt toolchain can produce the RPM.
- Prefer the SailfishOS Cursor skill when scaffolding or debugging SFOS-specific issues.
- After RPM install over SSH, enable user units for `harbour-muoto-launcher-icond` and `harbour-muoto-install-listener` if D-Bus name `org.muoto.Launcher1` is missing — see [testing](docs/devel/testing.md).
- Do **not** commit device passwords, SSH keys, or personal dconf dumps. Test scripts may document `MUOTO_SUDO_PASS` as an env override only.

## Test and debug

- Manual and scripted checks: [docs/devel/testing.md](docs/devel/testing.md).
- Journals, D-Bus, dconf, common failures: [docs/devel/debugging.md](docs/devel/debugging.md).
- Device scripts live under `scripts/` (`device-test-3.2.sh`, `device-test-preupgrade-install.sh`, `pipeline-review-*.sh`).

## Conventions

- Match existing QML/C++ style; prefer minimal diffs.
- Only create git commits when the user asks.
- Do not add unsolicited markdown/docs unless the task is documentation (this file and `docs/devel/` are intentional).

## Key entry points

| Concern | Files |
| ------- | ----- |
| Icon apply / rebuild | `src/launcher/launchericonops.cpp` |
| Install / update re-theme | `src/listener/installlistener.cpp`, `src/listener/pktxwatch.cpp`, `service/harbour-muoto-update-icons` |
| Icon inplace / redirect | `src/launcher/iconupdater.cpp`, `desktopentry.cpp` |
| Lipstick watch re-arm | `src/launcher/launcherwatch.cpp` |
| apkd container readiness | `src/launcher/aliendalvikwatcher.cpp` |
| Path resolve (hicolor size / APK) | `src/launcher/iconresolve.cpp` |
| Overlay composite | `src/launcher/overlayiconprovider.cpp`, `overlayrender.cpp` |
| Folder silica icons | `src/launcher/folderambient.cpp` |
| Pack lookup | `src/launcher/harbourthemepack.cpp` |
| Font apply / restore | `src/gui/fontapplier.cpp`, `src/gui/themepackmodel.cpp` |
| Density unlock / restore | `src/ops/densityenabler.cpp`, `src/gui/helperclient.cpp`, `qml/pages/DensityPage.qml` |
| Mosaic home / theme work | `qml/pages/MainPage.qml`, `qml/components/ThemeWork.qml` |
| Icons / fonts configure | `qml/pages/IconsConfigurePage.qml`, `qml/pages/FontsConfigurePage.qml` |
| Dyn icons | `qml/pages/DynamicIconsPage.qml` |
| Quick app switching | `qml/pages/QuickSwitchPage.qml`, `qml/components/QuickSwitchPreview.qml` |
| Session D-Bus | `dbus/org.muoto.Launcher1.Themes.xml`, `src/launcher-daemon/main.cpp` |
