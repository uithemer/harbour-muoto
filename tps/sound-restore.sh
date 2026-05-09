#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
dir_jolla=/usr/share/sounds/jolla-ambient/stereo

# Check if a backup has been performed
if [ "$(ls $main/backup/sound)" ]; then

# Restore Jolla sounds
cp $main/backup/sound/* $dir_jolla

# Remove backuped Jolla sounds
rm $main/backup/sound/*

# Set no sound pack
echo default > $main/sound-current

fi

exit 0
