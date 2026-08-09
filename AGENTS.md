# AGENTS.md

Orientation for coding agents working on **harbour-muoto** (Sailfish OS theming app).

## What this is

Muoto lets users apply `harbour-themepack-*` icon/font packs and display density on Sailfish OS. From **3.2**, launcher icons are applied only by the session daemon `harbour-muoto-launcher-icond`. The root helperd is slim (density unlock + uninstall pack). Fonts apply in-process in the GUI as `defaultuser`.

## Docs map

| Audience | Where |
| -------- | ----- |
| End users | [docs/guide.md](docs/guide.md), [docs/themes.md](docs/themes.md), [docs/density.md](docs/density.md) |
| Pack authors | [docs/getstarted.md](docs/getstarted.md), [docs/icons.md](docs/icons.md), [docs/fonts.md](docs/fonts.md) |
| Maintainers / agents | [docs/devel/](docs/devel/) — [architecture](docs/devel/architecture.md), [testing](docs/devel/testing.md), [debugging](docs/devel/debugging.md), [automation](docs/devel/automation.md) |

**Before changing icon apply/restore**, read [docs/devel/architecture.md](docs/devel/architecture.md).

## Icons (summary)

- Session D-Bus `org.muoto.Launcher1.Themes` (`ApplyIcons` / `RestoreIcons`).
- **ApplyIcons** sets `activeIconPack` / `iconOverlay`, `rebuildIconUpdaters()`, then `FolderAmbient::apply` (no stock intermediate).
- **Hybrid write model** (`IconUpdater` ctor):
  - **Hicolor** (`/usr/share/icons/hicolor/…/apps/`) → **inplace**: keep `Icon=harbour-foo`, replace the single resolved launcher-size PNG, touch desktop. Fingerprint in dconf detects “our” bytes.
  - **APK bridge** (`~/.local/share/apkd-bridge/launcherIcon/`) → **redirect**: unique `launcher-icons/<desktop>-<msecs>.png` + `Icon=` rewrite (absolute paths need a new path for Lipstick to refresh).
  - **Overlay** (missing pack icons only): `RedirectOnly` updater, but hicolor still forces inplace via the rule above.
- Pack lookup (`HarbourThemePack`): prefer `native|apk/<iconSizeLauncher>x…/`, else largest available pack size; jolla uses `jolla/z<pixelRatio>/`. Pack trees under `/usr/share/harbour-themepack-*` may be **symlinks** into `~/.themepack/` — follow them when debugging “empty native/”.
- Overlay composites pack `overlay/*.png` onto **stock** (backup if live was already themed). Not used when the pack already has that app.
- Only the **launcher-resolved** hicolor size is written (e.g. 172); other sizes (512, …) stay stock by design.
- Manifest + `saved-id` for restore; folder tiles via `FolderAmbient` (silica `icon-launcher-folder-*`).
- Per-app dconf `launcher/applications/<desktop>/provider`: only `dynamic-icon://` (clock/calendar) is honored.

## Homescreen icon refresh (do not “just overwrite the PNG”)

Lipstick caches launcher artwork by the desktop `Icon=` string. Overwriting bytes at a path Lipstick already resolved often leaves the grid stuck (no lipstick restart). Muoto’s refresh trick (`IconUpdater`):

1. **Redirect (APK / non-hicolor)** — write  
   `/usr/share/harbour-muoto/launcher-icons/<desktopBase>-<msecs>.png`  
   (`generateIconPath`), set `Icon=` to that path. A **new path** busts the cache.
2. **Inplace (hicolor)** — replace via temp `*.muoto-write.png` then rename onto the live path; `Icon=` name unchanged; **`futimens` the `.desktop`** so Lipstick reloads the named icon.
3. **Always touch the `.desktop`** after PNG / `Icon=` changes (`touchFile` / `futimens`).
4. Avoid deleting a PNG while `Icon=` still names it (`inotify_add_watch` ENOENT → frozen tile until lipstick restart).

Implementation: `src/launcher/iconupdater.cpp`, `iconresolve.cpp`, `overlayiconprovider.cpp`, `folderambient.cpp`.

## Fonts apply / restore

- Unprivileged, in the **GUI process** via `FontApplier` (`src/gui/fontapplier.cpp`), driven by `ThemePackModel::applyTheme` / `restoreTheme`.
- Confirm / Restore on the Themes tab call these (icons go through `Helper` → launcher-icond in parallel).
- Apply: read pack `font/` (+ optional `font-nonlatin/`), write `~/.config/fontconfig/conf.d/99-muoto.conf`, run `fc-cache`; sets `activeFontPack` in dconf.
- Restore: remove that conf (and related state), `fc-cache`, clear `activeFontPack`.
- User docs: [docs/fonts.md](docs/fonts.md). UI: `qml/pages/ConfirmPage.qml`, `RestorePage.qml`, `ThemesTabContent.qml`.

## Display density apply / restore

- **Unlock:** Display density tab calls `Helper.densityEnable()` → system bus `org.muoto.Muoto1` → helperd `DensityEnable` (moves vendor dconf locks so user keys can change). Implemented in `src/ops/densityenabler.cpp` / daemon adaptor.
- **Apply:** UI writes user dconf (`desktop/sailfish/silica/theme_pixel_ratio`, launcher icon size keys) once unlocked — see `qml/components/DensityTabContent.qml`.
- **Restore:** Themes-style restore on the density tab → `ThemePackModel::restoreDpi` → `DensityEnabler::restoreDensity` (reset selected keys / re-lock as designed).
- User docs: [docs/density.md](docs/density.md).

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
| Icon inplace / redirect | `src/launcher/iconupdater.cpp`, `desktopentry.cpp` |
| Path resolve (hicolor size / APK) | `src/launcher/iconresolve.cpp` |
| Overlay composite | `src/launcher/overlayiconprovider.cpp`, `overlayrender.cpp` |
| Folder silica icons | `src/launcher/folderambient.cpp` |
| Pack lookup | `src/launcher/harbourthemepack.cpp` |
| Font apply / restore | `src/gui/fontapplier.cpp`, `src/gui/themepackmodel.cpp` |
| Density unlock / restore | `src/ops/densityenabler.cpp`, `src/gui/helperclient.cpp`, `qml/components/DensityTabContent.qml` |
| Confirm / Themes UI | `qml/pages/ConfirmPage.qml`, `qml/components/ThemesTabContent.qml` |
| Dyn icons tab | `qml/components/DynamicIconsTabContent.qml` |
| Session D-Bus | `dbus/org.muoto.Launcher1.Themes.xml`, `src/launcher-daemon/main.cpp` |
