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
dir_apk=/var/lib/apkd

# Native icons
if [[ ! -f $pack/type || $(<$pack/type) != "android" ]]; then

	nativeCap=( "256x256" "172x172" "128x128" "108x108" "86x86" )

	for ((i=0;i<${#nativeCap[@]};++i)); do
	for ((j=i;j<${#nativeCap[@]};++j)); do
	# if there are native icons
	if [ -d $pack/native/${nativeCap[j]}/apps ]; then
		# List icons not in the theme
		diff -r $dir_native/${nativeCap[i]}/apps $pack/native/${nativeCap[j]}/apps | grep 'Only in /usr/share/icons' | awk '{print $4}' > $main/tmp/${nativeCap[i]}.overlay
		break
	fi
	done
	done

	for ((i=0;i<${#nativeCap[@]};++i)); do
	for file in $(<$main/tmp/${nativeCap[i]}.overlay); do 
	# Convert icons with ImageMagick
	find $pack/overlay/ -type f -name "*.png" | shuf -n 1 |\
	   convert \( @- -scale ${nativeCap[i]} -gravity Center \) \( $dir_native/${nativeCap[i]}/apps/$file -scale 60%x60% -gravity Center \) -composite -gravity Center -geometry ${nativeCap[i]} $main/tmp/$file
	# Move icons
	mv "$main/tmp/$file" $dir_native/${nativeCap[i]}/apps
	done
	done

fi

# if there are Android icons
apkCap=( "192x192" "128x128" "86x86" )
apkSize=( "122x122" "78x78" "56x56" )

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

# Save current icon pack
rm $main/icon-current
echo $iconpack > $main/icon-current

# Clean tmp directory
rm -r $main/tmp/*.png
rm -r $main/tmp/*.overlay
rm -r $main/tmp/*.overlaydroid
