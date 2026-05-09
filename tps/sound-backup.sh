#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
dir_jolla=/usr/share/sounds/jolla-ambient/stereo

mkdir -p $main/tmp
mkdir -p $main/backup
mkdir -p $main/backup/sound

# Copy Jolla sounds
cp $dir_jolla/* $main/backup/sound/

exit 0
