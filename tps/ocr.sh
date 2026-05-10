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

$main/restore_dpr.sh
$main/restore_iz.sh

if [ -d /usr/share/sailfishos-uithemer ]; then
	dconf write /desktop/lipstick/sailfishos-uithemer/activeIconPack "'default'"
	dconf write /desktop/lipstick/sailfishos-uithemer/activeFontPack "'default'"
	dconf write /desktop/lipstick/sailfishos-uithemer/autoUpdate 0
fi
