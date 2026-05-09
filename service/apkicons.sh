#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
iconpack=$(<$main/icon-current)
pack=/usr/share/harbour-themepack-$iconpack
dir_jolla=/usr/share/themes/sailfish-default/meegotouch
dir_native=/usr/share/icons/hicolor
dir_apk=/var/lib/apkd
source $main/config.shlib

# If Android support is installed
if [ -d "$dir_apk" ]; then
	apkCap=( "192x192" "128x128" "86x86" )
	apkSize=( "122x122" "78x78" "56x56" )

	# RESTORE
	rsync -a --existing --ignore-times $main/backup/icons/apk/ $dir_apk/

	# BACKUP
	rsync -a --ignore-existing $dir_apk/ $main/backup/icons/apk/

	# APPLY
	for ((i=0;i<${#apkCap[@]};++i)); do
	for ((j=i;j<${#apkCap[@]};++j)); do
		# if there are Android icons
		if [ -d $pack/apk/${apkCap[j]} ]; then
			# Perform copy of existing icons
			rsync -a --existing $pack/apk/${apkCap[j]}/ $dir_apk/
			break 3
		fi
	done
	done

	if [[ "$(config_get iconoverlay)" == "1" ]]; then
	 # Apply overlay
	if [[ -d $pack/overlay && "$(ls $pack/overlay)" ]]; then

		for ((i=0;i<${#apkCap[@]};++i)); do
		# if there are Android icons
		if [ -d $pack/apk/${apkCap[i]} ]; then
			# List icons not in the theme
			diff -r $dir_apk $pack/apk/${apkCap[i]} | grep 'Only in /var/lib/apkd' | awk '{print $4}' > $main/tmp/${apkCap[i]}.overlaydroid
		fi
		done

		if [[ ! -f $main/tmp/*.overlaydroid && $(<$pack/type) == "android" ]]; then
		   ls $dir_apk > $main/tmp/192x192.overlaydroid
		fi

		for ((i=0;i<${#apkCap[@]};++i)); do
		if [ -f $main/tmp/${apkCap[i]}.overlaydroid ]; then
			for file in $(<$main/tmp/${apkCap[i]}.overlaydroid); do 
			# Convert icons with ImageMagick
			find $pack/overlay/ -type f -name "*.png" | shuf -n 1 |\
			convert \( @- -scale ${apkCap[i]} -gravity Center \) \( $dir_apk/$file -scale ${apkSize[i]} -gravity Center \) -composite -gravity Center -geometry ${apkCap[i]} $main/tmp/$file
			# Move icons
			mv "$main/tmp/$file" $dir_apk
			done
			break 3
		fi
		done
	fi
	fi

	touch /usr/share/applications/*.desktop

fi

exit 0
