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

## OS update guard

While Sailfish OS is upgrading, Muoto must not re-apply icons (that raced with pre-upgrade restore). Detection uses signals SFOS actually exposes:

| Signal | Used by |
| ------ | ------- |
| `/run/defaultuser/osupdate_running` | `muoto_os_update_running`, `OsUpdateGuard`, `harbour-muoto-update-icons.service` `ConditionPathExists` |
| `system-update.target` active | Shell + C++ (when flag absent but upgrade target is up) |
| `sailfish-upgrade-ui.service` active | Shell + C++ |

**Not gated:** `harbour-muoto-oneshot-restore` still runs `RestoreIcons` before `sailfish-upgrade-ui` (stock restore is intentional during update).

During **Settings → Sailfish OS update**, expect `harbour-muoto-oneshot-restore` first (dconf → `default`), listener `apply skipped (guard)` on app installs, and `update-icons: skip (OS update in progress)` if the boot script is triggered manually.

```bash
ls -l /run/defaultuser/osupdate_running
systemctl is-active system-update.target sailfish-upgrade-ui.service
```

After reboot the flag should be gone; boot `update-icons` no-ops until a theme is applied again in the app.

## Waits and timeouts

Shell scripts do **not** wrap restore in an external `timeout` during RPM uninstall. Limits are layered:

| Limit | Value | What it bounds |
| ----- | ----- | -------------- |
| Per icon op (flock) | **180s** | Boot `update-icons` and pre-upgrade `oneshot-restore` (`ApplyIcons` / `RestoreIcons`) |
| Per icon op (flock) | **60s** | `harbour-muoto-oneshot-restore --uninstall` only |
| Lock “did not start” | **~15s** | Poll until helperd holds `icon-ops.lock`; else fail fast (busy / rejected) |
| Helperd bus name | **15s** | Root: `systemctl start harbour-muoto-helperd`. defaultuser: optional `StartService` (if supported), `Introspect` to activate, then poll; if still down, icon-op `dbus-send` activates (no PIN, no hard fail) |
| Icon op retry gap | **3s** | Sleep between one retry on failed restore or apply |
| systemd unit | **600s** | `TimeoutStartSec` on `harbour-muoto-update-icons.service` and `harbour-muoto-oneshot-restore.service` (whole oneshot run) |

**Flock semantics:** scripts wait until `/usr/share/harbour-muoto/icon-ops.lock` is free again. That matches helperd holding the lock for the whole pipeline job (see `FileLock` in C++). It means “operation finished,” not a second read of the D-Bus `OperationCompleted` success flag (the GUI still uses that signal).

**RPM uninstall:** `%preun` runs `harbour-muoto-oneshot-restore --uninstall` with no `timeout` and no `|| true`. If restore fails when `backup/icons` has PNGs, the script exits non-zero and the package stays installed.

## Device test checklist

1. Apply a theme in Muoto (`activeIconPack` ≠ `default`).
2. Install a native app (`pkcon install …`) or APK — icons should re-theme within ~2 s.
3. Restart AppSupport — `containerReady: true` should trigger apply.
4. Reboot — boot oneshot re-applies if theme still active.
5. System update: confirm `/run/defaultuser/osupdate_running` and/or upgrade units active; `harbour-muoto-oneshot-restore` runs (dconf `default`); auto-apply and `update-icons` are skipped; helperd `ApplyIcons` returns “upgrade in progress”.
6. After upgrade, boot apply no-ops until theme applied again in the app.
7. **Remove Muoto (RPM):** `%preun` stops `harbour-muoto-update-icons` and disables `harbour-muoto-install-listener`, then runs `harbour-muoto-oneshot-restore --uninstall`. Close the Muoto app before uninstall if a theme apply is in progress.

### Automated scripts

| Script | What |
| ------ | ---- |
| `scripts/device-test-3.2.sh` | Smoke: units, cap, manifest, `update-icons`; `--destructive` restore |
| `scripts/device-test-preupgrade-install.sh` | **T-20** pre-upgrade + **T-21** install/upgrade re-theme + **T-22** silica folder ambient |
| `scripts/pipeline-review-full.sh p10` / `p11` / `p12` | T-20 / T-21 / T-22 |

```bash
# On device (defaultuser). Copy the script from the repo if not present.
bash device-test-preupgrade-install.sh --pack haiku
# Or separately:
bash device-test-preupgrade-install.sh --pack haiku --skip-install --skip-folder   # T-20 only
bash device-test-preupgrade-install.sh --pack haiku --skip-preupgrade --skip-folder # T-21 only
bash device-test-preupgrade-install.sh --pack haiku --skip-preupgrade --skip-install # T-22 only
```

**T-20 pre-upgrade (`harbour-muoto-oneshot-restore`)**

| Check | Expect |
| ----- | ------ |
| Unit wiring | `Before=sailfish-upgrade-ui.service`, `WantedBy=system-update.target`, drop-in wants the oneshot |
| After apply + `systemctl start harbour-muoto-oneshot-restore` | `activeIconPack=default`, `iconOverlay=false`, manifest gone, generated PNGs cleared, Jolla `Icon=` back to stock names (e.g. fingerterm → `icon-launcher-shell`) |

**T-21 install/upgrade re-theme (`harbour-muoto-install-listener`)**

| Check | Expect |
| ----- | ------ |
| Listener active | `harbour-muoto-install-listener` running |
| `pkcon install --allow-reinstall` probe pkg (default `harbour-file-browser`) | Journal shows `muoto-listener` trigger / `update-icons finished` |
| After ~debounce+apply | Pack still active, probe app themed (manifest and/or generated `Icon=`), other launchers still themed |
| With `/run/defaultuser/osupdate_running` | Apply skipped (guard) |

**T-22 silica folder ambient**

| Check | Expect |
| ----- | ------ |
| Apply pack with overlay | Backup under `backup/folder-icons/<z>/icon-launcher-folder-01.png`; live silica PNG checksum changes when pack/overlay has assets |
| Leftover `Folder*.directory` with `Icon=` under `launcher-icons/` | Normalized back to `icon-launcher-folder-NN` |
| RestoreIcons | Live checksum restored; `backup/folder-icons` removed |

Override probe with `MUOTO_PROBE_PKG` / `MUOTO_PROBE_DESKTOP`. Sudo password: `MUOTO_SUDO_PASS` (default `rootme`).

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

# Listener status + live journal (script stderr forwarded as muoto: lines)
MUOTO_UID=$(id -u defaultuser)
export XDG_RUNTIME_DIR=/run/user/$MUOTO_UID
systemctl --user status harbour-muoto-install-listener
journalctl --user -f 2>/dev/null | grep -E 'muoto-listener|muoto:' \
  || journalctl -f | grep -E 'muoto-listener|muoto:'
```

If the listener is not active after RPM install from SSH, enable manually:

```bash
systemctl --user enable --now harbour-muoto-install-listener.service
```
