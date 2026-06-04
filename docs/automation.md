# Muoto auto-apply automation

Headless icon re-apply after app installs and at boot, plus a full stock restore before system upgrade.

## Components

| Artifact | Role |
| -------- | ---- |
| `/usr/bin/harbour-muoto-update-icons` | Read dconf → `ApplyIcons` (cover-sync); waits on `icon-ops.lock` via flock |
| `service/muoto-dbus-wait.sh` | Shared D-Bus helpers, backup probe, flock wait, `su defaultuser` dconf |
| `/usr/bin/harbour-muoto-oneshot-restore` | Pre-upgrade (no args) or RPM uninstall (`--uninstall`): fonts, conditional `RestoreIcons`, dconf, density, vendor locks |
| `harbour-muoto-update-icons.service` | Boot oneshot (runs as **root**; dconf via `su defaultuser`) |
| `harbour-muoto-oneshot-restore.service` | Before `sailfish-upgrade-ui` |
| `harbour-muoto-install-listener` | User D-Bus hooks → exec `harbour-muoto-update-icons` as **defaultuser** |
| `org.muoto.Muoto1` helperd | D-Bus activation on demand; `ApplyIcons` blocked during OS update |

## Waits and timeouts

Shell scripts do **not** wrap restore in an external `timeout` during RPM uninstall. Limits are layered:

| Limit | Value | What it bounds |
| ----- | ----- | -------------- |
| Per icon op (flock) | **180s** | Boot `update-icons` and pre-upgrade `oneshot-restore` (`ApplyIcons` / `RestoreIcons`) |
| Per icon op (flock) | **60s** | `harbour-muoto-oneshot-restore --uninstall` only |
| Lock “did not start” | **~15s** | Poll until helperd holds `icon-ops.lock`; else fail fast (busy / rejected) |
| Helperd bus name | **15s** | `muoto_ensure_helperd`: root uses `systemctl start harbour-muoto-helperd`; defaultuser (install listener) uses D-Bus `StartService` (no security-code prompt) |
| Icon op retry gap | **3s** | Sleep between one retry on failed restore or apply |
| systemd unit | **600s** | `TimeoutStartSec` on `harbour-muoto-update-icons.service` and `harbour-muoto-oneshot-restore.service` (whole oneshot run) |

**Flock semantics:** scripts wait until `/usr/share/harbour-muoto/icon-ops.lock` is free again. That matches helperd holding the lock for the whole pipeline job (see `FileLock` in C++). It means “operation finished,” not a second read of the D-Bus `OperationCompleted` success flag (the GUI still uses that signal).

**RPM uninstall:** `%preun` runs `harbour-muoto-oneshot-restore --uninstall` with no `timeout` and no `|| true`. If restore fails when `backup/icons` has PNGs, the script exits non-zero and the package stays installed.

## Device test checklist

1. Apply a theme in Muoto (`activeIconPack` ≠ `default`).
2. Install a native app (`pkcon install …`) or APK — icons should re-theme within ~2 s.
3. Restart AppSupport — `containerReady: true` should trigger apply.
4. Reboot — boot oneshot re-applies if theme still active.
5. System update triggers `harbour-muoto-oneshot-restore.service` as **root** (no `sudo` package) — stock icons/fonts/density; dconf `default`.
6. After upgrade, boot apply no-ops until theme applied again in the app.
7. **Remove Muoto (RPM):** `%preun` stops `harbour-muoto-update-icons` and disables `harbour-muoto-install-listener`, then runs `harbour-muoto-oneshot-restore --uninstall`. If `/usr/share/harbour-muoto/backup/icons` contains PNGs, `RestoreIcons` must succeed or the transaction aborts and the package stays installed. With no backup PNGs, icon D-Bus restore is skipped (fonts, dconf, density still run). Close the Muoto app before uninstall if a theme apply is in progress.

### Uninstall behaviour

| `backup/icons` | `RestoreIcons` | RPM if restore fails |
| -------------- | -------------- | -------------------- |
| Has PNGs       | Yes (helperd)  | **Aborts** — Muoto remains installed |
| Empty / missing | Skipped       | Removes if fonts/dconf succeed |

If Storeman or `pkcon` reports “system management is locked” on an **older** build, **restart the phone**, then update Muoto or retry uninstall.

```bash
# Manual apply as root (same as boot unit; theme must be active in dconf)
/usr/bin/harbour-muoto-update-icons

# Manual pre-upgrade restore as root (e.g. devel-su shell on device — not sudo, which is
# not installed by default). Needs defaultuser session: /run/user/<uid>/dbus/user_bus_socket
/usr/bin/harbour-muoto-oneshot-restore

# Same restore path as RPM uninstall (from devel-su)
/usr/bin/harbour-muoto-oneshot-restore --uninstall

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
