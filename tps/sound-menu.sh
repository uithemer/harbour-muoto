#!/bin/bash

# Make sure only root can run the script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root. Please gain root privileges and try again." 1>&2
   exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer

# Generate menu
find -L /usr/share/harbour-themepack-* -type d -maxdepth 1 -iname "sound" -print0 | xargs -0 -n1 dirname | sort -u | cut -c30- > $main/tmp/sound.menu

while :
do
    clear
    cat<<EOF
 Theme pack support for Sailfish OS
 ==================================

 Please enter your choice:
 ----------------------------------
   (A)pply sound theme
   (R)estore
   (B)ack
 ----------------------------------
 Current sound pack: $(<$main/sound-current)

EOF
    read -n1 -s
    case "$REPLY" in
    "A"|"a")  cat $main/tmp/sound.menu
		echo " "
		read -p "Please enter the sound pack name or 'q' to exit and press enter: " choice
		case "$choice" in 
		q|Q ) echo "ok"; sleep 1;;
		* ) echo "applying $choice"
		# Check if a backup has been performed
		if [ "$(ls $main/backup/sound)" ]; then
		$main/sound-restore.sh
		$main/sound-backup.sh
		$main/sound-run.sh $choice
		else
		$main/sound-backup.sh
		$main/sound-run.sh $choice
		fi
		if [ -d /usr/share/sailfishos-uithemer ]; then
		dconf write /desktop/lipstick/sailfishos-uithemer/activeSoundPack "'$choice'"
		fi
		echo "done!"; sleep 1 ;;
		esac ;;
    "R"|"r")  echo "This will restore your default sound pack. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) $main/sound-restore.sh
		if [ -d /usr/share/sailfishos-uithemer ]; then
		dconf write /desktop/lipstick/sailfishos-uithemer/activeSoundPack "'default'"
		fi
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
     "B"|"b")  rm -r $main/tmp/*; exit ;;
     * )  echo "invalid option"; sleep 1 ;;
    esac
    sleep 1
done
