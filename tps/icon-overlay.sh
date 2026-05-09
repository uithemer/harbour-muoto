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
mkdir -p "$dir_apk"

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

# Android launcher icons (flat apkd-bridge directory)
apkSize="192x192"
apkInner="122x122"
: > "$main/tmp/apk.overlaydroid"

if [[ -f $pack/type && $(<$pack/type) == "android" ]]; then
	shopt -s nullglob
	for f in "$dir_apk"/*.png; do
		base=$(basename "$f")
		echo "$base" >> "$main/tmp/apk.overlaydroid"
	done
	shopt -u nullglob
else
	shopt -s nullglob
	for f in "$dir_apk"/*.png; do
		[ -e "$f" ] || continue
		base=$(basename "$f")
		found=0
		for size in 192x192 128x128 86x86; do
			[ -e "$pack/apk/$size/$base" ] && { found=1; break; }
		done
		[ $found -eq 0 ] && echo "$base" >> "$main/tmp/apk.overlaydroid"
	done
	shopt -u nullglob
fi

if [ -s "$main/tmp/apk.overlaydroid" ] && [ -d "$pack/overlay" ] && [ "$(ls -A "$pack/overlay" 2>/dev/null)" ]; then
	while IFS= read -r file; do
		[ -n "$file" ] || continue
		find "$pack/overlay/" -type f -name "*.png" | shuf -n 1 |\
			convert \( @- -scale ${apkSize} -gravity Center \) \
				\( "$dir_apk/$file" -scale ${apkInner} -gravity Center \) \
				-composite -gravity Center -geometry ${apkSize} "$main/tmp/$file"
		mv "$main/tmp/$file" "$dir_apk/"
	done < "$main/tmp/apk.overlaydroid"
	chown -R defaultuser:defaultuser "$dir_apk" 2>/dev/null || true
fi

# Save current icon pack
rm $main/icon-current
echo $iconpack > $main/icon-current

# Clean tmp directory
rm -r $main/tmp/*.png
rm -r $main/tmp/*.overlay
rm -r $main/tmp/*.overlaydroid
