---
layout: default
title: Home
nav_order: 1
description: "Enables customization of icons, fonts, sounds and pixel density in Sailfish OS"
permalink: /
---

# UI Themer

UI Themer lets customize icons, fonts, sounds and pixel density in Sailfish OS. It bundles the former **Theme pack support** engine (shell scripts, systemd services, and the `themepacksupport` CLI) in a single package.

[![GitHub license](https://img.shields.io/github/license/uithemer/sailfishos-uithemer.svg)](https://github.com/uithemer/sailfishos-uithemer/blob/master/LICENSE) [![GitHub issues](https://img.shields.io/github/issues/uithemer/sailfishos-uithemer.svg)](https://github.com/uithemer/sailfishos-uithemer/issues) [![GitHub releases](https://img.shields.io/github/release/uithemer/sailfishos-uithemer.svg)](https://github.com/uithemer/sailfishos-uithemer/releases/latest) [![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://paypal.me/fravaccaro)

## Features

<a href="docs/screenshot1.png"><img width="25%" style="float: left;" src="docs/screenshot1.png" /></a> <a href="docs/screenshot2.png"><img width="25%" style="float: left;" src="docs/screenshot2.png" /></a> <a href="docs/screenshot3.png"><img width="25%" style="float: left;" src="docs/screenshot3.png" /></a> <a href="docs/screenshot4.png"><img width="25%" style="float: left;" src="docs/screenshot4.png" /></a>
<br style="clear: both; height:5px;" />

- Icon theming.
- Icon overlay.
- Font theming.
- Sound theming.
- Change device pixel ratio.
- Change DPI for Alien Dalvik.
- Change icon size.
- Recovery tools.

## Usage guide

A usage guide is available [here](docs/guide).

## Create custom theme packs

Documentation on how to create theme packs is in this repository under [`docs/getstarted.md`](docs/getstarted.md) (also published with the project site).

## Translate

Languages supported:

[![Languages supported](https://www.transifex.com/_/charts/redirects/fravaccaro/ui-themer/image_png/sailfishos-uithemerts)](https://www.transifex.com/fravaccaro/ui-themer/dashboard/)

Request a new language or contribute to existing languages on the [Transifex project page](https://www.transifex.com/fravaccaro/ui-themer/dashboard/).

## Builds

Builds for armv7hl and i486 available on [OpenRepos](https://openrepos.net/content/fravaccaro/ui-themer).

### Custom builds

You can change the defaults of UI Themer in your custom build. More info [here](docs/custom-build).

## Roadmap

Roadmap and features will be tracked on the [Trello dashboard](https://trello.com/b/WwLwj2eu).

## Credits

- Theme pack support engine (bash, systemd, CLI) by fravaccaro (formerly separate `themepacksupport-sailfishos` package).
- Partially based on [Icon pack support GUI](https://github.com/RikudouSage/sailfish-iconpacksupport-gui).
- App icon by [Free Vectors](http://www.freevectors.com/blue-painting-roller/).
- Iconography by [Retinaicons](https://www.flaticon.com/authors/retinaicons).
- Keyboard navigation based on [Piepmatz](https://github.com/Wunderfitz/harbour-piepmatz) by Sebastian Wolf.
- Thanks to Dax89 for helping with C++ and QML code, this app would not exist without him.
- Thanks to Eugenio_g7 for helping with the *One-click restore* service.
- Thanks to LQS for helping with the Android DPI on the Xperia XA2.
- Thanks to all the testers for being brave and patient.
