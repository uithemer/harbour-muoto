---
layout: default
title: Icon pack guidelines
parent: Get started
nav_order: 1
---

# Icon pack guidelines

Here you'll find info on how to create icons compatible with Theme pack support.

## Icon paths in Sailfish OS

Icons are located in three folders:
* `/usr/share/themes/sailfish-default/meegotouch` for Jolla stock icons.
* `/usr/share/icons/hicolor` for native icons.
* `/var/lib/apkd/` for Android icons.

## Create your icons

1. Create icons with the image editor of your choice. 
2. Icons go in `jolla` if they are Jolla stock icons, `native` if they are 3rd party icons or `apk` if they are Android apps icons. Size and place them accordingly with the folder, so icons 86x86px will go in `86x86/apps`; for Jolla stock icons, 86x86px in `z1.0/icons`, 129x129px in `z1.5/icons` and 172x172px in `z2.0/icons`; Android ones go in `apk/86x86` and `apk/128x128`.

### Jolla Ambient

Theme pack support also enables theming Jolla Ambient icons.

They are a set of icons displayed in applications that use default components, so theming Jolla Ambient would have effect on all native apps.

As for Jolla stock apps, they are stored in `/usr/share/themes/sailfish-default/meegotouch`, conveniently divided into:

* *graphic-* general icons, from regular controls (like busy indicators, pulley menu handles and poweroff/lock icons) to stock apps graphics (banners, cover graphics and weather icons).
* *graphic-service-*, *icon-m-service-*, *icon-s-service-*: icons for account settings.
* *icon-camera-*: icons used in the camera app (focus, flash, etc).
* *icon-cover-*: icons used in cover actions (pause, play, new message).
* *icon-direction-*: icons used in Jolla Maps app.
* *icon-l-*, *icon-m-*, *icon-s-*: general-purpose icons, used throughout the OS.
* *icon-launcher-*: apart from Jolla stock app icons, there are the canvas for folder icons.
* *icon-lock-*: notification icon showed in the lockscreen.
* *icon-lock-emergency-call*/*icon-lockscreen-emergency-call*: for the emergency call button in numeric pad.
* *icon-status-*/*icon-system-*: icons showed in the status bar (airplane mode, signal, bluetooth, alarm, gps, etc).

#### z1.0

Jolla Ambient icons placed in `z1.0/icons` will be sized:

* *graphic-* general icon of various dimensions.
* *graphic-service-*: 135x135px. Other sizes: *icon-m-service-* (64x64px) and *icon-s-service-* (32x32px).
* *icon-camera-*: mostly 48x48px, except for the shutter icon, 64x64px.
* *icon-cover-*: 32x32px.
* *icon-direction-*: 128x128px.
* *icon-l-*: 96x96px. The same icons are available also as *icon-m-* (64x64px, except *icon-m-incoming-call* and *icon-m-missed-call*, which are 42x42px) and *icon-s-* (32x32px).
* *icon-launcher-*: 86x86px.
* *icon-lock-* 32x32px.
* *icon-lock-emergency-call*/*icon-lockscreen-emergency-call*: 64x64px.
* *icon-status-*/*icon-system-*: 24x24px.

#### z1.5

Jolla Ambient icons placed in `z1.5/icons` will be sized:

* *graphic-* general icon of various dimensions. Their size in pixels from `z1.0/icons` is a multiple of 1.5.
* *graphic-service-*: 203x203px. Other sizes: *icon-m-service-* (96x96px) and *icon-s-service-* (48x48px).
* *icon-camera-*: mostly 72x72px, except for the shutter icon, 96x96px.
* *icon-cover-*: 48x48px.
* *icon-direction-*: 192x192px.
* *icon-l-*: 144x144px. The same icons are available also as *icon-m-* (96x96px, except *icon-m-incoming-call* and *icon-m-missed-call*, which are 63x63px) and *icon-s-* (48x48px).
* *icon-launcher-*: 129x129px.
* *icon-lock-* 48x48px.
* *icon-lock-emergency-call*/*icon-lockscreen-emergency-call*: 96x96px.
* *icon-status-*/*icon-system-*: 36x36px.

#### z2.0

Jolla Ambient icons placed in `z1.0/icons` will be sized:

* *graphic-* general icon of various dimensions. Their size in pixels from `z1.0/icons` is a multiple of 2.
* *graphic-service-*: 270x270px. Other sizes: *icon-m-service-* (128x128px) and *icon-s-service-* (64x64px).
* *icon-camera-*: mostly 96x96px, except for the shutter icon, 128x128px.
* *icon-cover-*: 64x64px.
* *icon-direction-*: 256x256px.
* *icon-l-*: 192x192px. The same icons are available also as *icon-m-* (128x128px, except *icon-m-incoming-call* and *icon-m-missed-call*, which are 84x84px) and *icon-s-* (48x48px).
* *icon-launcher-*: 172x172px.
* *icon-lock-* 64x64px.
* *icon-lock-emergency-call*/*icon-lockscreen-emergency-call*: 128x128px.
* *icon-status-*/*icon-system-*: 48x48px.

#### References

* Additional info are available in the [Sailfish Documentation](https://sailfishos.org/develop/docs/jolla-ambient/). 
* Some nice alternative icon sets are available [here](http://www.flaticon.com/).

### DynCal

[DynCal](https://github.com/fravaccaro/harbour-dyncal)-skinning support built-in. To add it, place your icons in the `dyncal/256x256` folder, renaming icons as `dd.png` where `dd` is the day (from 01 to 31) or as `mmdd.png` where `mm` is the month for holiday icons.

The theme pack support will check automatically if DynCal is installed.

### DynClock

[DynClock](https://github.com/fravaccaro/harbour-dynclock)-skinning support is built-in. To add it:

1. Download `bg.png` `hour.png` and `minute.png` from [here](https://github.com/fravaccaro/harbour-dynclock/tree/master/harbour-dynclock/usr/share/harbour-dynclock).
2. Edit them as you like.
3. Put them in the `dynclock/256x256` folder.

The theme pack support will check automatically if DynClock is installed.

## Overlays

If your theme sports regular icon shape, you can create an overlay, that can be applied automatically for icons missing from you theme. Create a png sized 512x512px and place it into the 'overlay' folder.

### Android apps

If you want to apply an overlay to Android apps only, place in the root folder of your theme a file named 'type', containing a single line with the text 'android' (without apices). Note: this works with themes containing ONLY overlays.
