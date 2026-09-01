---
layout: default
title: Testing
parent: Developers
nav_order: 2
---

# Testing

How to verify Muoto on a Sailfish OS device. Prefer real hardware; emulator coverage for Lipstick/APK is limited.

## Build and deploy

Use the [Sailfish SDK](https://developer.sailfishos.com/develop/sdk/) (`sfdk`):

```bash
~/SailfishOS/bin/sfdk config target=SailfishOS-<version>-aarch64   # or armv7hl
~/SailfishOS/bin/sfdk build
~/SailfishOS/bin/sfdk deploy --sdk
```

After installing an RPM **over SSH** (no active GUI session at install time), user units may be missing. As `defaultuser`:

```bash
ln -sf /usr/lib/systemd/user/harbour-muoto-launcher-icond.service \
  ~/.config/systemd/user/
ln -sf /usr/lib/systemd/user/harbour-muoto-install-listener.service \
  ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now harbour-muoto-launcher-icond harbour-muoto-install-listener
```

Confirm session D-Bus name `org.muoto.Launcher1` is owned before calling `ApplyIcons`.

## Manual smoke

1. Open Muoto → **Icons** tile → apply a pack (and optionally overlay). Use **Fonts** for font packs.
2. Check homescreen: Jolla, harbour apps, and APK icons look themed.
3. Switch to a second pack; icons should follow without a full reboot (APK may need pulley **Restart homescreen** if Lipstick caches absolute `Icon=`).
4. **Icons** page or **Dynamic icons** tile: enable clock or calendar when the pack ships `dynclock/` / `dyncal/`.
5. On Icons (and Fonts if needed), select **Default** and **Apply** → stock; `activeIconPack` is `default`.
6. Display density: change scale, apply, restore.

## Automated device scripts

Copy scripts from the repo onto the device (or run via SSH). Run as **defaultuser** unless noted. Sudo password override: `MUOTO_SUDO_PASS` (default `rootme` on typical devel images).

| Script | Purpose |
| ------ | ------- |
| `scripts/device-test-3.2.sh` | Smoke: units, `cap_dac_override`, manifest, `update-icons`; `--destructive` runs restore |
| `scripts/device-test-preupgrade-install.sh` | **T-20** pre-upgrade, **T-21** install re-theme, **T-22** folder ambient, **T-23** dynamic icons |
| `scripts/pipeline-review-full.sh` | Pipeline slices `p1`…`p13` (includes T-20…T-23 as `p10`…`p13`) |
| `scripts/pipeline-review-p5-p9.sh` | Subset of pipeline cases |

```bash
# On device
bash device-test-3.2.sh
bash device-test-3.2.sh --destructive

bash device-test-preupgrade-install.sh --pack haiku
bash device-test-preupgrade-install.sh --pack haiku --skip-install --skip-folder --skip-dyn   # T-20
bash device-test-preupgrade-install.sh --pack haiku --skip-preupgrade --skip-folder --skip-dyn # T-21

# Or from host
ssh defaultuser@<device> 'bash -s' < scripts/device-test-3.2.sh
```

Probe package overrides for T-21: `MUOTO_PROBE_PKG`, `MUOTO_PROBE_DESKTOP`.

Auto-apply / upgrade / uninstall expectations are detailed under [Automation](automation).

## What “good” looks like after ApplyIcons

- `dconf read /apps/harbour-muoto/activeIconPack` is the pack short name.
- `/usr/share/harbour-muoto/launcher-manifest.json` exists.
- APK / redirect entries: PNGs under `launcher-icons/` and `Icon=/usr/share/harbour-muoto/launcher-icons/…`.
- Native inplace: `Icon=` may still be `harbour-…` while the launcher-size hicolor PNG and manifest `mode=inplace` reflect the pack.
- Live silica under `graphic-*` is **unchanged** (no bulk ambient); folder `icon-launcher-folder-*` may change when folder theming applies.
