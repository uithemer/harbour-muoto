---
layout: default
title: Debugging
parent: Developers
nav_order: 3
---

# Debugging

On-device checks when icons, auto-apply, or density misbehave. Run as `defaultuser` unless noted; use `devel-su` / `sudo` for root-only paths.

## Services and capabilities

```bash
systemctl --user status harbour-muoto-launcher-icond harbour-muoto-install-listener
systemctl --user is-active harbour-muoto-launcher-icond

# File capability on the daemon binary
getcap /usr/libexec/harbour-muoto-launcher-icond
# Or CapEff bit 1 (CAP_DAC_OVERRIDE) in /proc/<pid>/status while running
```

If `org.muoto.Launcher1` has no owner after RPM install over SSH, symlink and enable the user unit (see [Testing](testing#build-and-deploy)).

## D-Bus

**Session** (icons):

```bash
dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus \
  org.freedesktop.DBus.GetNameOwner string:org.muoto.Launcher1

dbus-send --session --print-reply --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
  org.freedesktop.DBus.Introspectable.Introspect | head

# Apply / restore (signature: pack, runPack, overlay)
dbus-send --session --type=method_call --print-reply \
  --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
  org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true

dbus-send --session --type=method_call --print-reply \
  --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
  org.muoto.Launcher1.Themes.RestoreIcons
```

**System** (density / uninstall pack): `org.muoto.Muoto1` — activated on demand via helperd.

## dconf and files

```bash
dconf dump /apps/harbour-muoto/

grep -H '^Icon=' /usr/share/applications/fingerterm.desktop \
  /usr/share/applications/voicecall-ui.desktop \
  /usr/share/applications/jolla-messages.desktop

grep -H '^Icon=' ~/.local/share/applications/apkd_launcher_*.desktop | head

ls -lt /usr/share/harbour-muoto/launcher-icons/ | head
wc -c /usr/share/harbour-muoto/launcher-manifest.json
```

Expect **APK** (and many jolla) themed launchers to use absolute paths under `launcher-icons/`. **Native/hicolor** themed apps often keep `Icon=harbour-foo` while only the launcher-size hicolor PNG changed — compare that file (and the manifest) before assuming apply failed. A stock-looking `Icon=` name alone does not mean “not themed.”

## Journal

```bash
journalctl --user -u harbour-muoto-launcher-icond --no-pager -n 80
journalctl --user -u harbour-muoto-install-listener --no-pager -n 40
journalctl --user --no-pager --since '5 min ago' | grep -iE 'muoto-launcher|muoto-listener|ApplyIcons|RestoreIcons'
```

Look for `ApplyIcons start/done`, `rebuildIconUpdaters`, `file not found`, and `skip re-entrant`.

## Common failure modes

| Symptom | Likely cause | Check |
| ------- | ------------ | ----- |
| Nothing themes after install | `launcher-icond` not running / no D-Bus name | User unit symlink + `enable --now`; GetNameOwner |
| Half icons look stock after pack switch | Lipstick cache; pack lacks assets (overlay off); or inplace only updated launcher size | Check manifest `mode`; compare hicolor launcher-size vs 512; restart homescreen if needed |
| APK icons stuck on previous pack | Lipstick lost its inotify watch on the desktop (apkd regenerated it) | Confirm on-disk `Icon=` changed, then check the watch (below); look for `re-armed launcher watches` in the icond journal |
| APK icons stock after an Android container restart | `containerReady` not received, or apkd clobbered after the retry | icond journal should show `apkd containerReady` then `refreshApkIcons`; check the property with `dbus-send --session --print-reply --dest=com.jolla.apkd /com/jolla/apkd org.freedesktop.DBus.Properties.Get string:com.jolla.apkd string:containerReady` |
| New app installed but icon is stock | Listener ignored the PackageKit role, or `update-icons` no-op'd on a double-prefixed pack dir | Listener journal: `role=` / `roleRelevant=` (expect `10`/`11`/`22` with `true`); `update-icons` must log `ApplyIcons`, not `missing /usr/share/harbour-themepack-harbour-themepack-…`. Icond should also log `refreshNewDesktops pending= N themed= N` |
| Apply returns “upgrade in progress” | OS update guard | `/run/defaultuser/osupdate_running`, `system-update.target` — see [Automation](automation) |
| Apply returns “busy” | `icon-ops.lock` held | Wait; check for stuck icond |
| Blank tiles | Empty / deleted hicolor leftover | Manifest restore; `rpm -V` / reinstall app |
| Folder icons stay themed after restore | Stock backups were under wiped `backup/icons/` or poisoned after re-apply | After 3.5.2 upgrade: `harbour-muoto-repair-folder-icons` runs (save pack → RestoreIcons → pkcon reinstall owning graphics z packages → reapply → removes its own unit). Manual: `devel-su /usr/bin/harbour-muoto-repair-folder-icons`. Or `pkcon install --allow-reinstall $(rpm -qf …/icon-launcher-folder-01.png)` |
| Dyn clock/calendar not live | Flags off or pack without `dynclock`/`dyncal` | dconf `dynamic*Enabled`; pack dirs |

## Is Lipstick still watching a desktop entry?

An `Icon=` rewrite only reaches the grid if Lipstick holds an inotify watch on that `.desktop`. Its watch descriptors list the watched inodes in hex:

```sh
LP=$(pgrep -f '^/usr/bin/lipstick')
D=~/.local/share/applications/apkd_launcher_org_telegram_messenger-org_telegram_messenger_DefaultIcon.desktop
sudo grep -h '^inotify' /proc/$LP/fdinfo/* | grep "ino:$(printf '%x' $(stat -c %i "$D")) "
```

No match means the watch is gone and the tile cannot refresh until it is re-armed — see the re-arm section in [Architecture](architecture). Beware inode reuse when comparing across a rename; a surer signal is that `~/.config/nemomobile/lipstick.conf` gets rewritten (`savePositions()`) every time Lipstick processes a desktop change.

## Architecture pointer

Data flow and source map: [Architecture](architecture).
