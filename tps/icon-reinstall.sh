#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer

$main/icon-restore.sh

pkcon repo-set-data jolla refresh-now true

pkcon install --allow-reinstall -y sailfish-content-graphics-default-z1.0-base sailfish-content-graphics-default-z1.25-base sailfish-content-graphics-default-z1.5-base sailfish-content-graphics-default-z1.75-base sailfish-content-graphics-default-z2.0-base

ls -d /usr/share/themes/sailfish-default/meegotouch/[z]* | cut -c47- | tr "\n" " " > $main/tmp/default-icons
for i in $(<$main/tmp/default-icons); do pkcon install --allow-reinstall -y sailfish-content-graphics-closed-$i; done

ls /usr/share/icons/hicolor/86x86/apps -1 | sed -e 's/\..*$//' > $main/tmp/native-reinstall
for file in $(<$main/tmp/native-reinstall); do
	pkcon install --allow-reinstall -y $file
done

if [ -d /usr/share/sailfishos-uithemer ]; then
	dconf write /desktop/lipstick/sailfishos-uithemer/activeIconPack "'default'"
fi

# Clean tmp directory
if [ "$(ls $main/tmp)" ]; then
	rm $main/tmp/*
fi

exit 0
