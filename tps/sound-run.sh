#!/bin/bash

# Usage ./run.sh [soundpackname]

# Set sound pack name variable
soundpack=$1
# Is the variable empty?
if [ -z "$soundpack" ]; then
    exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer
pack=/usr/share/harbour-themepack-$soundpack
dir_jolla=/usr/share/sounds/jolla-ambient/stereo

# Check if a backup has been performed
if [ "$(ls $main/backup/sound)" ]; then

# Check if there are Jolla sounds
if [ -d "$pack/sound" ]; then
	cp $pack/sound/*.wav $dir_jolla/
fi

# Clean tmp directory
if [ "$(ls $main/tmp)" ]; then
	rm $main/tmp/*
fi

# Save current sound pack
rm $main/sound-current
echo $soundpack > $main/sound-current

# Warm about backup not performed
else
echo "Run backup first!"
fi
exit 0
