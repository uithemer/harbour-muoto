#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
dir_jolla=/usr/share/themes/sailfish-default/meegotouch
dir_native=/usr/share/icons/hicolor
dir_apk=/home/defaultuser/.local/share/apkd-bridge/launcherIcon
source $main/config.shlib

# Disable service
if [[ "$(config_get autoupd)" == "1" ]]; then
	systemctl disable themepacksupport-autoupdate.timer
	systemctl stop themepacksupport-autoupdate.timer
	systemctl disable themepacksupport-autoupdate.service
	systemctl stop themepacksupport-autoupdate.service
fi

# Jolla icons
jollaCap=( "z2.0" "z1.75" "z1.5-large" "z1.5" "z1.25" "z1.0" )

for ((i=0;i<${#jollaCap[@]};++i)); do
	# Perform copy of existing icons
	if [ -d $main/backup/jolla/${jollaCap[i]}/icons/ ]; then
		rsync -a --existing --ignore-times $main/backup/jolla/${jollaCap[i]}/icons/ $dir_jolla/${jollaCap[i]}/icons/
	fi
	rsync -a --existing --ignore-times $main/backup/icons/jolla/${jollaCap[i]}/icons/ $dir_jolla/${jollaCap[i]}/icons/
done

# Native icons
nativeCap=( "256x256" "172x172" "128x128" "108x108" "86x86" )

for ((i=0;i<${#nativeCap[@]};++i)); do
	# Perform copy of existing icons
	if [ -d $main/backup/native/${nativeCap[i]}/apps/ ]; then
		rsync -a --existing --ignore-times $main/backup/native/${nativeCap[i]}/apps/ $dir_native/${nativeCap[i]}/apps/
	fi
	rsync -a --existing --ignore-times $main/backup/icons/native/${nativeCap[i]}/apps/ $dir_native/${nativeCap[i]}/apps/
done

# Android launcher icons (apkd-bridge)
mkdir -p "$dir_apk"
if [ -d $main/backup/apk/ ]; then
	rsync -a --existing --ignore-times $main/backup/apk/ $dir_apk/
fi
rsync -a --existing --ignore-times $main/backup/icons/apk/ $dir_apk/
chown -R defaultuser:defaultuser "$dir_apk" 2>/dev/null || true

# If DynCal is installed
if [ -d "/usr/share/harbour-dyncal" ]; then
	if [ "$(ls $main/backup/dyncal)" ]; then
		if [ -d $main/backup/dyncal/ ]; then
			cp $main/backup/dyncal/* /usr/share/harbour-dyncal/icons/
		fi
		cp $main/backup/icons/dyncal/* /usr/share/harbour-dyncal/icons/
	fi
fi

# If DynClock is installed
if [ -d "/usr/share/harbour-dynclock" ]; then
	# Restore hd config
	if [ -s "/usr/share/harbour-dynclock/dynclock.cfg" ]; then
		echo res="128" > /usr/share/harbour-dynclock/dynclock.cfg
	fi
	if [ "$(ls $main/backup/dynclock)" ]; then
		if [ -d $main/backup/dynclock/ ]; then
			cp $main/backup/dynclock/* /usr/share/harbour-dynclock/
		fi
		cp $main/backup/icons/dynclock/* /usr/share/harbour-dynclock/
	fi
fi

rm -rf $main/backup/jolla
rm -rf $main/backup/native
rm -rf $main/backup/apk
rm -rf $main/backup/dynclock
rm -rf $main/backup/dyncal

rm -rf $main/backup/icons

# Set no icon pack
echo default > $main/icon-current

exit 0
