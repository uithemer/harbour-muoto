#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
iconpack=$(<$main/icon-current)
pack=/usr/share/harbour-themepack-$iconpack
dir_jolla=/usr/share/themes/sailfish-default/meegotouch
dir_native=/usr/share/icons/hicolor
source $main/config.shlib

mkdir -p $main/tmp
mkdir -p $main/backup
mkdir -p $main/backup/icons
mkdir -p $main/backup/icons/jolla
mkdir -p $main/backup/icons/native
mkdir -p $main/backup/icons/apk
mkdir -p $main/backup/icons/dyncal
mkdir -p $main/backup/icons/dynclock

# RESTORE

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

# BACKUP

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

# APPLY

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

# Overlay

if [[ "$(config_get iconoverlay)" == "1" ]]; then
	$main/icon-overlay.sh $iconpack
fi

touch /usr/share/applications/*.desktop

exit 0
