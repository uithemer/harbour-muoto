#!/bin/bash

main=/usr/share/sailfishos-uithemer

echo "Starting one-click restore"

$main/disable-autoupdate.sh

/usr/bin/sailfishos-uithemer-reassert --restore || true

$main/font-restore.sh
$main/sound-restore.sh

$main/restore_dpr.sh
$main/restore_adpi.sh
$main/restore_iz.sh
$main/disable-dpi.sh

if [ -d /usr/share/sailfishos-uithemer ]; then
	dconf write /desktop/lipstick/sailfishos-uithemer/activeIconPack "'default'"
	dconf write /desktop/lipstick/sailfishos-uithemer/activeFontPack "'default'"
	dconf write /desktop/lipstick/sailfishos-uithemer/densityEnabled false
	dconf write /desktop/lipstick/sailfishos-uithemer/autoUpdate 0
fi
