#!/bin/bash

# Make sure only root can run the script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root. Please gain root privileges and try again." 1>&2
   exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer
source $main/config.shlib

cat $main/tmp/icon.menu
echo " "
read -p "Please enter the icon pack name or 'q' to exit and press enter: " iconpack
case "$iconpack" in 
    q|Q ) echo "ok" ; sleep 1 ;;
    * ) echo "applying $iconpack"
	$main/icon-restore.sh
	$main/icon-backup.sh
	$main/icon-run.sh $iconpack
	config_write iconoverlay 0
	touch /usr/share/applications/*.desktop
	if [ -d /usr/share/sailfishos-uithemer ]; then
	dconf write /desktop/lipstick/sailfishos-uithemer/activeIconPack "'$iconpack'"
	fi
	echo "done!" ; sleep 1 ;;
esac 

if [ -d "/usr/share/harbour-themepack-$iconpack/overlay" ]; then
	if [ "$(ls /usr/share/harbour-themepack-$iconpack/overlay)" ]; then
		echo "Do you want to apply an overlay for icons not available the theme? y/N "
		read -n1 -s choice
		case "$choice" in 
			y|Y ) echo "applying $iconpack overlay"
			$main/icon-overlay.sh $iconpack
			config_write iconoverlay 1
			touch /usr/share/applications/*.desktop
			if [ -d /usr/share/sailfishos-uithemer ]; then
			dconf write /desktop/lipstick/sailfishos-uithemer/activeIconPack "'$iconpack'"
			fi
			echo "done!"; sleep 1 ;;
			* ) echo "ok"; sleep 1 ;;
		esac 
	fi
fi

exit
