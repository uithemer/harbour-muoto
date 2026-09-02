---
layout: default
title: Quick app switching
parent: Using Muoto
nav_order: 5
---

# Quick app switching

Quick app switching is a hidden Sailfish OS gesture that jumps straight back to the previous app, similar to pressing <kbd>Alt</kbd>+<kbd>Tab</kbd> once on a computer. Unlike Alt+Tab you can only jump to the previous app, not cycle further back.

The **Quick app switching** tile on the home mosaic turns it on and off, so you do not have to edit dconf over SSH.

## Using the gesture

From inside any app, peek from the edge of the screen:

* about **three centimetres**, and
* **slowly** — take more than half a second.

Once both thresholds are passed, the previous app's cover highlights on the switcher. Lift your finger to switch to it, or peek back towards the edge of the screen to cancel.

A normal quick peek still works as usual and takes you home, so the gesture does not replace anything you already use.

## Turning it on

Open the **Quick app switching** tile, flip the switch, and swipe **Apply**. The tile subtitle shows **On** or **Off**. **Cancel** discards the change.

The setting takes effect immediately — no homescreen restart and no reboot.

## Notes

* This is an **experimental** Sailfish OS setting. Jolla documents it as a key that may change behaviour or disappear as Sailfish OS evolves, so a future OS release could make the switch do nothing.
* Muoto writes the standard user key `/desktop/sailfish/experimental/quickAppToggleGesture`. Nothing else on the system is modified, and the setting is independent of icon, font, and density theming.
* Because it is a plain user setting, it survives reboots and Muoto updates, and it is left untouched if you uninstall Muoto. To clear it by hand: `dconf reset /desktop/sailfish/experimental/quickAppToggleGesture`.
