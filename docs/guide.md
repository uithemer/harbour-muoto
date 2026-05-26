---
layout: default
title: Usage guide
nav_order: 2
permalink: docs/guide
---

# Usage guide

UI Themer lets you customize icons, fonts, and display density on Sailfish OS.

## Before a system update

Revert themes and display-density changes **before** updating Sailfish OS. If you forgot, open UI Themer and use **Restore theme** from the Themes tab pulley menu (icons and/or fonts), then restore display density from the **Display density** tab.

## Main window

The app opens on two tabs:

* **Themes** — icon and font packs from OpenRepos or other sources
* **Display density** — pixel ratio, Android DPI, and launcher icon size

Documentation for creating packs is on the [Get started](getstarted) page ([full site](https://uithemer.github.io/sailfishos-uithemer/docs/getstarted)).

## Themes tab

Installed `harbour-themepack-*` packages appear in the list.

**Apply a theme:** tap a pack, choose icons, overlay, and/or fonts (and font weight if offered), then confirm. You can combine packs — for example icons from one pack and fonts from another (apply each part separately).

**Icon overlay:** if the pack supports it, enable overlay to fill in icons not included in the pack using composited frames from the pack `overlay/` folder.

**Homescreen refresh:** after apply or restore, launcher icons usually update automatically. If icons look stale, enable *Restart homescreen* in the confirm dialog, or use **Restart homescreen** from the pulley menu (with remorse).

**Restore theme:** pulley → *Restore theme* — choose whether to restore icons and/or fonts to stock.

**Other pulley actions:**

* *About UI Themer* — version, changelog, link to pack documentation
* *Support UI Themer* — optional support dialog
* *Restart first run wizard*
* *Restart homescreen*
* *Download more themes* — opens Storeman search when Storeman is installed

**Uninstall:** use the pack item menu to remove an RPM; if that pack is active for icons, UI Themer restores stock icons first.

## Display density tab

Adjust how much UI fits on screen:

* **Theme pixel ratio** — Sailfish UI density
* **Android DPI** — separate setting for Alien Dalvik apps
* **Launcher icon size** — includes a *System default* preset to reset dconf

Use the pulley menu on this tab for *About*, *Restart homescreen*, and density **restore** (same pattern as the Themes tab).

A full device restart may still be needed on some hardware (e.g. Xperia XA2) for Android DPI changes to take effect everywhere.

## Cover

The app cover shows the active icon/font pack when set. After applying from the main UI, the cover can reflect your choices; icon apply and restore run through the privileged helper service.

## Further help

* [Theme pack guidelines](https://uithemer.github.io/sailfishos-uithemer/docs/getstarted)
* [Report an issue](https://github.com/uithemer/sailfishos-uithemer/issues)
