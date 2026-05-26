---
layout: default
title: Font pack guidelines
parent: Get started
nav_order: 2
---

# Font pack guidelines

How to create fonts compatible with **UI Themer**.

Sailfish OS uses separate font configuration for the system UI and Alien Dalvik; theme packs supply files under `font/` and `font-nonlatin/`. UI Themer writes `~/.config/fontconfig/conf.d/99-uithemer.conf` and runs `fc-cache` for the current user when a font pack is applied.

Download TrueType (`.ttf`) or collection (`.ttc`) fonts of your choice. One typeface family per pack is typical, with multiple weights (Bold, Regular, Light, etc.).

Note: results may vary if the fonts lack glyphs for some characters.

## Latin / UI weights

1. Rename files as `Regular.ttf`, `Light.ttf`, `ExtraLight.ttf`, and so on (matching the weight names you expose in the apply dialog).
2. Place them in the pack `font/` directory.

## Non-Latin fonts

Place files in `font-nonlatin/`. UI Themer maps these filenames to fontconfig language tags:

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
