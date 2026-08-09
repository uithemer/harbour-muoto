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
| APK icons stuck on previous pack | Absolute `Icon=` cached by Lipstick | Confirm on-disk `Icon=` path/md5 changed; restart homescreen |
| Apply returns “upgrade in progress” | OS update guard | `/run/defaultuser/osupdate_running`, `system-update.target` — see [Automation](automation) |
| Apply returns “busy” | `icon-ops.lock` held | Wait; check for stuck icond |
| Blank tiles | Empty / deleted hicolor leftover | Manifest restore; `rpm -V` / reinstall app |
| Dyn clock/calendar not live | Flags off or pack without `dynclock`/`dyncal` | dconf `dynamic*Enabled`; pack dirs |

## Architecture pointer

Data flow and source map: [Architecture](architecture).
