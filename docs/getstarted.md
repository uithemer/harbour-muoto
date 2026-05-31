---
layout: default
title: Create theme packs
nav_order: 3
permalink: docs/getstarted
has_children: true
has_toc: true
---

# Create theme packs

Information on how to create theme packs compatible with **Muoto**. Use the [example package](https://github.com/uithemer/harbour-themepack-example) as a reference — it includes starter templates for icon-only, font-only, and combined packs (`icon-theme/`, `font-theme/`, `full-theme/`), each with a `theme/` asset tree and `rpm/*.spec` for building with the Sailfish SDK.

For **using** the app (apply themes, display density, restore), see **[Using Muoto](guide)**.

## Requisites

* [Sailfish SDK](https://developer.sailfishos.com/develop/sdk/) with `sfdk` on your PATH (often `~/SailfishOS/bin/sfdk`)
* Basic knowledge of the Linux terminal; familiarity with RPM packaging is helpful but not required for the sfdk workflow

## Icons, fonts, sounds

Read the page for the part of the theme you are creating, then return here for packaging and release.

* [Icons](icons)
* [Fonts](fonts)
* [Sounds](sounds) — **not supported** in current Muoto releases (format kept for reference and migration)

## Project layout

Each theme pack project has two top-level directories:

```
harbour-themepack-mypackage/
├── theme/           ← pack assets (icons, fonts, package file)
└── rpm/
    └── harbour-themepack-mypackage.spec
```

Place icons and fonts under `theme/` (see the [icon](icons) and [font](fonts) guidelines). The `theme/package` file holds the human-readable display name (one line). The spec's `%install` section copies `theme/` into `/usr/share/harbour-themepack-mypackage/` at build time.

## .spec file

Open `rpm/harbour-themepack-mypackage.spec` and edit these lines:

* **Name**: the name used in the system to identify your package.
* **Version**: the version of the package.
* **Release**: the issue of the version.
* **Summary**: an intelligible name for your package.
* **Vendor**: your name or nickname.
* **Packager**: your name or nickname and email (optional).
* **URL**: your blog or website (optional).
* **description**: description for your package.
* **changelog**: the changelog of your package.

### Notes

* The package name must start with `harbour-themepack-`, e.g. `harbour-themepack-mypackage`.
* The file `theme/package` contains the human-readable name of your theme pack, e.g. *My theme pack*. Keep it on one line.

## Building

1. Fork or copy a template from [harbour-themepack-example](https://github.com/uithemer/harbour-themepack-example) (`icon-theme/`, `font-theme/`, or `full-theme/`).
2. Place your assets under `theme/` and edit metadata in `rpm/*.spec`.
3. Select an SDK build target and run `sfdk build` (see below).
4. Publish the resulting `RPMS/*.noarch.rpm` on [OpenRepos](https://openrepos.net).

### Select a build target

List installed SDK tooling and available targets:

```bash
~/SailfishOS/bin/sfdk tools list
```

Each target name encodes the Sailfish OS version and CPU architecture, for example `SailfishOS-5.0.0.62-armv7hl` or `SailfishOS-5.0.0.62-armv7hl.default`. Pick the target that matches your SDK installation and device (most phones use `armv7hl`; newer devices may use `aarch64`). Theme packs are `BuildArch: noarch` — the target arch only selects which SDK chroot builds the RPM, not the package contents.

Use the exact target string shown by `tools list` (including a `.default` suffix if present).

### Build commands

From your pack project root:

```bash
~/SailfishOS/bin/sfdk config target=SailfishOS-5.0.0.62-armv7hl
~/SailfishOS/bin/sfdk config target   # verify active target
~/SailfishOS/bin/sfdk build
```

Output: `RPMS/harbour-themepack-mypackage-<version>-<release>.noarch.rpm`

Optional — install on a connected device via the SDK:

```bash
~/SailfishOS/bin/sfdk deploy --sdk
```

## Themepack helper

To resize SVG icons you can use [themepack-helper](https://github.com/uithemer/harbour-themepack-example/themehelper).

## Companion app

If you are familiar with the Sailfish SDK, you can use the [companion app](https://github.com/uithemer/harbour-themepack-example/tree/master/companion) model. It ships a small Sailfish app alongside the theme pack (icon requests, docs links, translations). Build it with `cd companion && sfdk build` — see the [companion section](https://github.com/uithemer/harbour-themepack-example#companion-app) in the example repo README.

## Releasing

You are ready to build and publish. The usual channel is [OpenRepos](https://openrepos.net).

In your package description or store notice, tell users to install **Muoto** from [OpenRepos](https://openrepos.net/content/fravaccaro/muoto-ui-themer) (this app replaces the old *Theme pack support* / `harbour-themepacksupport` and *UI Themer* / `sailfishos-uithemer` packages). Theme packs must follow the `harbour-themepack-*` naming convention and the layout described in the guidelines linked above.
