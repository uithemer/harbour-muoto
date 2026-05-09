#!/bin/bash

# Usage ./run.sh [iconpackname]

# Set icon pack name variable
iconpack=$1
# Is the variable empty?
if [ -z "$iconpack" ]; then
    exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer
pack=/usr/share/harbour-themepack-$iconpack
dir_jolla=/usr/share/themes/sailfish-default/meegotouch/icons

# Check if a backup has been performed
if [ "$(ls $main/backup/graphic)"  ]; then

# Check if there are graphic icons
if [ -d "$pack/graphic" ]; then

# Check if there are graphic icons
	if [ "$(ls $pack/graphic)" ]; then
		# List icon pack icons
		ls $pack/graphic > $main/tmp/pack_graphic
		# List graphic icons
		ls $dir_jolla > $main/tmp/sys_graphic
		#Check if there are common icons
		comm -1 -2 $main/tmp/pack_graphic $main/tmp/sys_graphic > $main/tmp/cp_graphic
		if [ -s "$main/tmp/cp_graphic" ]; then
			# Copy selected graphic icon pack
			for file in $(<$main/tmp/cp_graphic); do cp "$pack/graphic/$file" $dir_jolla; done
		fi
	fi

# Clean tmp directory
if [ "$(ls $main/tmp)" ]; then
	rm $main/tmp/*
fi

# Save current icon pack
rm $main/graphic-current
echo $iconpack > $main/graphic-current

# Warm about backup not performed
else
echo "Run backup first!"

fi
fi

exit 0
