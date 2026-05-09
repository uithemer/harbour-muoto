---
layout: default
title: Font pack guidelines
parent: Get started
nav_order: 2
---

# Font pack guidelines

Here you'll find info on how to create fonts compatible with Theme pack support.

In Sailfish OS there are two sets of fonts, system and Alien Dalvik, so you need to theme both.

First of all, download the TrueTypeFont (ttf) fonts of your choice. Only one font type is permitted, but 
as many font styles (Bold, Regular, Light) as you like.

Note: result may vary depending if the fonts you chose have some characters missing.

1. Rename your fonts as `Regular.ttf`, `Light.ttf`, `ExtraLight.ttf`, etc.
2. Place your fonts in `font`.

## Non-latin fonts

1. Rename your fonts as `Arabic.ttf`, `Armenian.ttf`, `Chinese.ttc`, `Devangari.ttf`, `Ethiopic.ttf`, 
`Georgian.ttf`, `Japanese.ttf`, `Hebrew.ttf`, `Naskh.ttf`, `Tamil.ttf` or `Thai.ttf`.
2. Place your fonts in `font-nonlatin`.

### Asian languages

For Asian languages, Sailfish OS uses a .ttc font archive format. In order to make your font compatible with it, your 'Chinese.ttc' should contain 3 fonts named as following:

* 'WenQuanYiZenHei-01.ttf'
* 'WenQuanYiZenHeiMono-02.ttf'
* 'WenQuanYiZenHeiSharp-03.ttf'

To achieve so, you can use softwares such as FontForge.
