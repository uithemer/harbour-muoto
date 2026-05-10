#!/bin/bash

main=/usr/share/sailfishos-uithemer

echo "Starting one-click restore"

$main/disable-autoupdate.sh

/usr/bin/sailfishos-uithemer-reassert --restore || true

# 2.5.0 font theming is now a per-user fontconfig conf file. Drop it and
# refresh the cache as defaultuser; no-op if absent.
f=/home/defaultuser/.config/fontconfig/conf.d/99-uithemer.conf
if [ -f "$f" ]; then
    rm -f "$f"
    su - defaultuser -c "fc-cache -f" || true
fi

# 2.5.6: restore_dpr.sh / restore_iz.sh were retired together with
# scripts/restore_dpi.sh; the equivalent dconf resets now run inline as
# defaultuser. icon_size_launcher had no real seed/snapshot anyway, so
# resetting it brings the launcher icon back to the vendor default.
su - defaultuser -c "dconf reset /desktop/sailfish/silica/theme_pixel_ratio" || true
su - defaultuser -c "dconf reset /desktop/sailfish/silica/icon_size_launcher" || true

if [ -d /usr/share/sailfishos-uithemer ]; then
	dconf write /desktop/lipstick/sailfishos-uithemer/activeIconPack "'default'"
	dconf write /desktop/lipstick/sailfishos-uithemer/activeFontPack "'default'"
	dconf write /desktop/lipstick/sailfishos-uithemer/autoUpdate 0
fi
