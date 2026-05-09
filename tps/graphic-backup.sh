#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
dir_jolla=/usr/share/themes/sailfish-default/meegotouch/icons

mkdir -p $main/tmp
mkdir -p $main/backup
mkdir -p $main/backup/graphic

# List graphic icons
cp $dir_jolla/* $main/backup/graphic

exit 0
