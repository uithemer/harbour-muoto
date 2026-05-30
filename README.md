---
layout: default
title: Home
nav_order: 1
description: "Enables customization of icons, fonts and pixel density in Sailfish OS"
permalink: /
---

# Muoto

Muoto lets you customize icons, fonts and pixel density in Sailfish OS. It bundles the former **Theme pack support** engine (systemd services, privileged helper, and compatibility with `harbour-themepack-*` packages) in a single app.

[![GitHub license](https://img.shields.io/github/license/uithemer/harbour-muoto.svg)](https://github.com/uithemer/harbour-muoto/blob/master/LICENSE) [![GitHub issues](https://img.shields.io/github/issues/uithemer/harbour-muoto.svg)](https://github.com/uithemer/harbour-muoto/issues) [![GitHub releases](https://img.shields.io/github/release/uithemer/harbour-muoto.svg)](https://github.com/uithemer/harbour-muoto/releases/latest) [![Donate on Liberapay](https://img.shields.io/badge/Donate-Liberapay-orange.svg)](https://liberapay.com/fravaccaro)

## Features

<a href="docs/screenshots/screenshot1.png"><img width="25%" style="float: left;" src="docs/screenshots/screenshot1.png" alt="Screenshot 1" /></a> <a href="docs/screenshots/screenshot2.png"><img width="25%" style="float: left;" src="docs/screenshots/screenshot2.png" alt="Screenshot 2" /></a> <a href="docs/screenshots/screenshot3.png"><img width="25%" style="float: left;" src="docs/screenshots/screenshot3.png" alt="Screenshot 3" /></a> <a href="docs/screenshots/screenshot4.png"><img width="25%" style="float: left;" src="docs/screenshots/screenshot4.png" alt="Screenshot 4" /></a>
<br style="clear: both; height:5px;" />

- Icon theming (native, Jolla, Android).
- Style missing app icons (theme look for apps not in the pack).
- Font theming.
- Display density (pixel ratio, Android DPI, launcher icon size).

## Using Muoto

[Using Muoto](docs/guide.md) — apply themes, display density, restore, and other app features.

## Create theme packs

[Create theme packs](docs/getstarted.md) — author documentation (icons, fonts, packaging).

## Translate

Request a new language or contribute on the [Transifex project page](https://explore.transifex.com/fravaccaro/ui-themer).

## Builds

Builds for aarch64, armv7hl and i486 are available on [OpenRepos](https://openrepos.net/content/fravaccaro/muoto-ui-themer).

## Migration from UI Themer

- Restore your current theme from **UI Themer** (`sailfishos-uithemer`) before swapping packages.
- Uninstall `sailfishos-uithemer`.
- Install `harbour-muoto` (also replaces the merged **Theme pack support** / `harbour-themepacksupport` package).
- Re-apply your preferred theme packs in Muoto.
- App settings are not auto-migrated; re-apply packs if needed after upgrading.

## Credits

- [Opal](https://github.com/Pretty-SFOS/opal) QML modules (About, Tabs, SupportMe, LinkHandler) by [Mirian Margiani](https://github.com/Pretty-SFOS/opal-about).
- Theme pack support engine by fravaccaro (formerly separate `themepacksupport-sailfishos` package).
- Partially based on [Icon pack support GUI](https://github.com/RikudouSage/sailfish-iconpacksupport-gui).
- App icon by [Free Vectors](http://www.freevectors.com/blue-painting-roller/).
- Iconography by [Retinaicons](https://www.flaticon.com/authors/retinaicons).
- Thanks to Dax89 for C++ and QML help.
- Thanks to Eugenio_g7 for the *One-click restore* service.
- Thanks to LQS for Android DPI on Xperia XA2.
- Thanks to [dt.iki.fi/sailfish-os-change-default-font](https://dt.iki.fi/sailfish-os-change-default-font).
- Thanks to all testers.
