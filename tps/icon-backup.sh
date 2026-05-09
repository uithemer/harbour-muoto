#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
dir_jolla=/usr/share/themes/sailfish-default/meegotouch
dir_native=/usr/share/icons/hicolor
dir_apk=/var/lib/apkd
source $main/config.shlib

# Disable service
if [[ "$(config_get autoupd)" == "1" ]]; then
	systemctl disable themepacksupport-autoupdate.timer
	systemctl stop themepacksupport-autoupdate.timer
	systemctl disable themepacksupport-autoupdate.service
	systemctl stop themepacksupport-autoupdate.service
fi

mkdir -p $main/tmp
mkdir -p $main/backup
mkdir -p $main/backup/icons
mkdir -p $main/backup/icons/jolla
mkdir -p $main/backup/icons/native
mkdir -p $main/backup/icons/apk
mkdir -p $main/backup/icons/dyncal
mkdir -p $main/backup/icons/dynclock

# Jolla icons
jollaCap=( "z2.0" "z1.75" "z1.5-large" "z1.5" "z1.25" "z1.0" )

for ((i=0;i<${#jollaCap[@]};++i)); do
	mkdir -p $main/backup/icons/jolla/${jollaCap[i]}/
	mkdir -p $main/backup/icons/jolla/${jollaCap[i]}/icons/
	# Perform copy of existing icons
	rsync -a --ignore-existing $dir_jolla/${jollaCap[i]}/icons/*.png $main/backup/icons/jolla/${jollaCap[i]}/icons/
done

# Native icons
nativeCap=( "256x256" "172x172" "128x128" "108x108" "86x86" )

for ((i=0;i<${#nativeCap[@]};++i)); do
	mkdir -p $main/backup/icons/native/${nativeCap[i]}/
	mkdir -p $main/backup/icons/native/${nativeCap[i]}/apps/
	# Perform copy of existing icons
	rsync -a --ignore-existing $dir_native/${nativeCap[i]}/apps/*.png $main/backup/icons/native/${nativeCap[i]}/apps/
done

# If Android support is installed
if [ -d "$dir_apk" ]; then
	rsync -a --ignore-existing $dir_apk/ $main/backup/icons/apk/
fi

# If DynCal is installed 
if [ -d "/usr/share/harbour-dyncal" ]; then
	rsync -a --ignore-existing /usr/share/harbour-dyncal/icons/*.* $main/backup/icons/dyncal/
fi

# If DynClock is installed 
if [ -d "/usr/share/harbour-dynclock" ]; then
	rsync -a --ignore-existing /usr/share/harbour-dynclock/*.png $main/backup/icons/dynclock/
fi

exit 0
