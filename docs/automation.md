# Muoto auto-apply automation

Headless icon re-apply after app installs and at boot, plus a full stock restore before system upgrade.

## Components

| Artifact | Role |
| -------- | ---- |
| `/usr/bin/harbour-muoto-update-icons` | Read dconf → `ApplyIcons` (cover-sync); waits for `OperationCompleted` |
| `/usr/bin/harbour-muoto-oneshot-restore` | Pre-upgrade: `RestoreIcons`, fonts, Muoto + silica dconf, vendor locks |
| `harbour-muoto-update-icons.service` | Boot oneshot (`User=defaultuser`) |
| `harbour-muoto-oneshot-restore.service` | Before `sailfish-upgrade-ui` |
| `harbour-muoto-install-listener` | User D-Bus hooks → exec update script |
| `org.muoto.Muoto1` helperd | D-Bus activation on demand; `ApplyIcons` blocked during OS update |

## Device test checklist

1. Apply a theme in Muoto (`activeIconPack` ≠ `default`).
2. Install a native app (`pkcon install …`) or APK — icons should re-theme within ~2 s.
3. Restart AppSupport — `containerReady: true` should trigger apply.
4. Reboot — boot oneshot re-applies if theme still active.
5. Run `harbour-muoto-oneshot-restore` as root before upgrade — stock icons/fonts/density; dconf `default`.
6. After upgrade, boot apply no-ops until theme applied again in the app.

```bash
# Manual apply (theme must be active in dconf)
/usr/bin/harbour-muoto-update-icons

# Manual pre-upgrade restore
sudo /usr/bin/harbour-muoto-oneshot-restore

# Listener status + live journal (after logging fix)
MUOTO_UID=$(id -u defaultuser)
export XDG_RUNTIME_DIR=/run/user/$MUOTO_UID
systemctl --user status harbour-muoto-install-listener
journalctl --user -f -t harbour-muoto-install-listener 2>/dev/null \
  || journalctl -f | grep muoto-listener
```

If the listener is not active after RPM install from SSH, enable manually:

```bash
systemctl --user enable --now harbour-muoto-install-listener.service
```
