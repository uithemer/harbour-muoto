#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
dir_jolla=/usr/share/themes/sailfish-default/meegotouch/icons

# Check if a backup has been performed
if [ "$(ls $main/backup/graphic)" ]; then

# List backuped graphic icons
ls $main/backup/graphic > $main/tmp/bk_graphic
# List graphic icons
ls $dir_jolla/ > $main/tmp/sys_graphic
#Check if there are common icons
comm -1 -2 $main/tmp/bk_graphic $main/tmp/sys_graphic > $main/tmp/cp_graphic
if [ -s "$main/tmp/cp_graphic" ]; then
	# Restore Jolla icons
	for file in $(<$main/tmp/cp_graphic); do cp "$main/backup/graphic/$file" $dir_jolla; done
fi

# Clean tmp directory
rm $main/tmp/*
if [ -s "$main/backup/cp_graphic*" ]; then
	rm $main/backup/cp_graphic*
fi

# Set no icon pack
echo default > $main/graphic-current

fi

exit 0
