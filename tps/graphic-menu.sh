#!/bin/bash

# Make sure only root can run the script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root. Please gain root privileges and try again." 1>&2
   exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer

# Generate menu
find -L /usr/share/harbour-themepack-* -type d -maxdepth 1 -iname "graphic" -print0 | xargs -0 -n1 dirname | sort -u | cut -c30- > $main/tmp/graphic.menu

while :
do
    clear
    cat<<EOF
 Theme pack support for Sailfish OS
 ==================================

 Please enter your choice:
 ----------------------------------
   (A)pply graphic theme
   (R)estore
   (B)ack
 ----------------------------------
 Current graphic pack: $(<$main/graphic-current)

EOF
    read -n1 -s
    case "$REPLY" in
    "A"|"a")  cat $main/tmp/graphic.menu
		echo " "
		read -p "Please enter the graphic pack name or 'q' to exit and press enter: " choice
		case "$choice" in 
		q|Q ) echo "ok"; sleep 1;;
		* ) echo "applying $choice"
		# Check if a backup has been performed
		if [ "$(ls $main/backup/graphic)" ]; then
		$main/graphic-restore.sh
		$main/graphic-backup.sh
		$main/graphic-run.sh $choice
		else
		$main/graphic-backup.sh
		$main/graphic-run.sh $choice
		fi
		echo "done!"; sleep 1 ;;
		esac ;;
    "R"|"r")  echo "This will restore your default graphic pack. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) $main/graphic-restore.sh
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
     "B"|"b")  rm -r $main/tmp/*; exit ;;
     * )  echo "invalid option"; sleep 1 ;;
    esac
    sleep 1
done
