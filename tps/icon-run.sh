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
dir_jolla=/usr/share/themes/sailfish-default/meegotouch
dir_native=/usr/share/icons/hicolor
dir_apk=/home/defaultuser/.local/share/apkd-bridge/launcherIcon
source $main/config.shlib

# Check if a backup has been performed
#if [ ! "$(ls $main/backup/icons)" ]; then
#	echo "Run backup first!"
#	exit 1
#fi

# Jolla icons
jollaCap=( "z2.0" "z1.75" "z1.5-large" "z1.5" "z1.25" "z1.0" )

for ((i=0;i<${#jollaCap[@]};++i)); do
for ((j=i;j<${#jollaCap[@]};++j)); do
	# if there are Jolla icons
	if [ -d $pack/jolla/${jollaCap[j]}/icons ]; then
		# Perform copy of existing icons
		rsync -a --existing $pack/jolla/${jollaCap[j]}/icons/ $dir_jolla/${jollaCap[i]}/icons/
		break
	fi
done
done

# Native icons
nativeCap=( "256x256" "172x172" "128x128" "108x108" "86x86" )

for ((i=0;i<${#nativeCap[@]};++i)); do
for ((j=i;j<${#nativeCap[@]};++j)); do
	# if there are native icons
	if [ -d $pack/native/${nativeCap[j]}/apps ]; then
		# Perform copy of existing icons
		rsync -a --existing $pack/native/${nativeCap[j]}/apps/ $dir_native/${nativeCap[i]}/apps/
		break
	fi
done
done

# Android launcher icons (apkd-bridge: flat PNGs in launcherIcon)
mkdir -p "$dir_apk"
apkCap=( "192x192" "128x128" "86x86" )
for ((i=0;i<${#apkCap[@]};++i)); do
for ((j=i;j<${#apkCap[@]};++j)); do
	if [ -d $pack/apk/${apkCap[j]} ]; then
		rsync -a --existing $pack/apk/${apkCap[j]}/ $dir_apk/
		chown -R defaultuser:defaultuser "$dir_apk" 2>/dev/null || true
		break 3
	fi
done
done

# If DynCal is installed
if [ -d "/usr/share/harbour-dyncal" ]; then
	# Check if there are DynCal icons
	if [ -d "$pack/dyncal/86x86" ]; then
		rm -rf /usr/share/harbour-dyncal/icons/*
		cp $pack/dyncal/86x86/*.* /usr/share/harbour-dyncal/icons/
		sh /usr/share/harbour-dyncal/harbour-dyncal.sh
	fi
fi

# If DynClock is installed
if [ -d "/usr/share/harbour-dynclock" ]; then
	# Check if there are DynClock icons
	if [ -d "$pack/dynclock/86x86" ]; then
		cp $pack/dynclock/86x86/*.* /usr/share/harbour-dynclock/
		echo res="43" > /usr/share/harbour-dynclock/dynclock.cfg
		sh /usr/share/harbour-dynclock/harbour-dynclock.sh
	fi
fi

# Save current icon pack
rm $main/icon-current
echo $iconpack > $main/icon-current

# Re-enable service if enabled
if [[ "$(config_get autoupd)" == "1" ]]; then
	systemctl enable themepacksupport-autoupdate.timer
	systemctl start themepacksupport-autoupdate.timer
	systemctl enable themepacksupport-autoupdate.service
	systemctl start themepacksupport-autoupdate.service
fi

exit 0
