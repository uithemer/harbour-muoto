#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
dir_jolla=/usr/share/fonts
dir_apk=/opt/alien/system/fonts

mkdir -p $main/tmp
mkdir -p $main/backup
mkdir -p $main/backup/font
mkdir -p $main/backup/font-droid
mkdir -p $main/backup/font-nonlatin

# Copy Jolla fonts
cp -R $dir_jolla/* $main/backup/font/

# If Android support is installed
if [ -d "$dir_apk" ]; then
	# Copy Android fonts
	cp -R $dir_apk/* $main/backup/font-droid/
fi

exit 0
