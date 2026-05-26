---
layout: default
title: Create theme packs
nav_order: 3
permalink: docs/getstarted
has_children: true
has_toc: true
---

# Create theme packs

Information on how to create theme packs compatible with **UI Themer**. Use the [dummy package](https://github.com/uithemer/harbour-themepack-dummy) as a reference — it includes the basic directory tree and an example spec file.

For **using** the app (apply themes, display density, restore), see **[Using UI Themer](guide)**.

## Requisites

* Basic knowledge of the Linux terminal and how to build RPM packages.

## Icons, fonts, sounds

Read the page for the part of the theme you are creating, then return here for packaging and release.

* [Icons](icons)
* [Fonts](fonts)
* [Sounds](sounds) — **not supported** in current UI Themer releases (format kept for reference and migration)

## .spec file

Open the `.spec` file and edit these lines:

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
* The file `package` contains the human-readable name of your theme pack, e.g. *My theme pack*. Keep it on one line.

## Building

[This guide](http://talk.maemo.org/showthread.php?t=92963) is a useful introduction to building Sailfish packages.

A theme pack is mostly icons and fonts, so build it **architecture agnostic**. Append `--target noarch` to rpmbuild:

`rpmbuild --bb --target noarch PATHOFTHESPECFILE/harbour-themepack-mypackage.spec`

## Themepack helper

To resize SVG icons you can use [themepack-helper](https://github.com/uithemer/themepack-helper).

## Companion app

If you are familiar with the Sailfish SDK, you can use the [companion app](https://github.com/uithemer/harbour-themepack-companion) model. It includes a script that lists missing icons and opens an email draft with a predefined address. Feel free to fork it for your own projects.

## Releasing

You are ready to build and publish. The usual channel is [OpenRepos](https://openrepos.net).

In your package description or store notice, tell users to install **UI Themer** (this app replaces the old standalone *Theme pack support* / `harbour-themepacksupport` package). Theme packs must follow the `harbour-themepack-*` naming convention and the layout described in the guidelines linked above.
