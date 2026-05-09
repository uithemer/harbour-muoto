#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
dir_jolla=/usr/share/fonts
dir_apk=/opt/alien/system/fonts

# Check if a backup has been performed
if [ "$(ls $main/backup/font)" -o "$(ls $main/backup/font-droid)" -o "$(ls $main/backup/font-nonlatin)" ]; then

# Restore Jolla fonts
cp -R $main/backup/font/* $dir_jolla/

# If Android support is installed
if [ -d "$dir_apk" ]; then
	# Restore Android fonts
	cp -rf $main/backup/font-droid/*.* $dir_apk/
fi

# Remove backuped Jolla fonts
if [ "$(ls $main/backup/font)" ]; then
	rm -rf $main/backup/font/*
fi

# Remove backuped Android fonts
if [ "$(ls $main/backup/font-droid)" ]; then
	rm -rf $main/backup/font-droid/*
fi

# Remove backuped nonlatin fonts
if [ "$(ls $main/backup/font-nonlatin)" ]; then
	rm -rf $main/backup/font-nonlatin/*
	if [ -d "$dir_jolla/nonlatin" ]; then
		rm -R $dir_jolla/nonlatin
	fi
fi

# Set no font pack
echo default > $main/font-current

fi

exit 0
