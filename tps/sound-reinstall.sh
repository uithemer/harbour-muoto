#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer

$main/sound-restore.sh

pkcon repo-set-data jolla refresh-now true
pkcon install --allow-reinstall -y jolla-ambient-sound-theme

if [ -d /usr/share/sailfishos-uithemer ]; then
	dconf write /desktop/lipstick/sailfishos-uithemer/activeSoundPack "'default'"
fi

# Clean tmp directory
if [ "$(ls $main/tmp)" ]; then
	rm $main/tmp/*
fi

exit 0
