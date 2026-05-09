#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer

$main/font-restore.sh

pkcon repo-set-data jolla refresh-now true
pkcon install --allow-reinstall -y sailfish-fonts

if [ -d /usr/share/sailfishos-uithemer ]; then
	dconf write /desktop/lipstick/sailfishos-uithemer/activeFontPack "'default'"
fi

# Clean tmp directory
if [ "$(ls $main/tmp)" ]; then
	rm $main/tmp/*
fi

exit 0
