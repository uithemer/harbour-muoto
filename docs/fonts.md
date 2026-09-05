---
layout: default
title: Font pack guidelines
parent: Create theme packs
nav_order: 2
---

# Font pack guidelines

How to create fonts compatible with **Muoto**.

Sailfish OS uses separate font configuration for the system UI and Alien Dalvik; theme packs supply files under `theme/font/` and `theme/font-nonlatin/`. When you apply a font pack, Muoto copies those files into `~/.local/share/fonts/muoto/` (readable by Sailjail apps), writes `~/.config/fontconfig/conf.d/99-muoto.conf` pointing at that staging directory, and runs `fc-cache` for the current user. Restore removes the conf and the staging tree. If you upgraded from Muoto before 3.2.2, reapply the font once so jailed apps can use it.

Download TrueType (`.ttf`) or collection (`.ttc`) fonts of your choice. One typeface family per pack is typical, with multiple weights (Bold, Regular, Light, etc.).

Note: results may vary if the fonts lack glyphs for some characters.

## Latin / UI weights

1. Rename files as `Regular.ttf`, `Light.ttf`, `ExtraLight.ttf`, and so on (matching the weight names you expose in the apply dialog).
2. Place them in `theme/font/`.

## Building

Copy the [`font-theme/`](https://github.com/uithemer/harbour-themepack-example/tree/master/font-theme) template from the example repo, add your fonts under `theme/`, edit `rpm/*.spec` metadata, then build with the Sailfish SDK — see [Building](getstarted.md#building) in the getstarted guide.

## Non-Latin fonts

Place files in `theme/font-nonlatin/`. Muoto maps these filenames to fontconfig language tags:

| Pack filename | Used for |
| ------------- | -------- |
| `Arabic.ttf` | Arabic (`ar`) |
| `Devanagari.ttf` | Hindi / Devanagari (`hi`) |
| `Chinese.ttc` | Chinese (`zh`) |
| `Japanese.ttf` | Japanese (`ja`) |
| `Hebrew.ttf` | Hebrew (`he`) |
| `Thai.ttf` | Thai (`th`) |

Other names (e.g. `Armenian.ttf`, `Georgian.ttf`, `Tamil.ttf`) may be present in older pack docs but are **not** applied by the current engine unless support is added later.

### Asian languages (Chinese.ttc)

Sailfish expects a `.ttc` archive for Chinese. Your `Chinese.ttc` should contain three fonts named:

* `WenQuanYiZenHei-01.ttf`
* `WenQuanYiZenHeiMono-02.ttf`
* `WenQuanYiZenHeiSharp-03.ttf`

You can build or edit these with tools such as [FontForge](https://fontforge.org/).
