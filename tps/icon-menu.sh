#!/bin/bash

# Make sure only root can run the script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root. Please gain root privileges and try again." 1>&2
   exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer

# Generate menu
find -L /usr/share/harbour-themepack-* -type d -maxdepth 1 -iname "jolla" -print0 | xargs -0 -n1 dirname | sort -u | cut -c30- > $main/tmp/icon-tmp1
find -L /usr/share/harbour-themepack-* -type d -maxdepth 1 -iname "native" -print0 | xargs -0 -n1 dirname | sort -u | cut -c30- > $main/tmp/icon-tmp2
find -L /usr/share/harbour-themepack-* -type d -maxdepth 1 -iname "apk" -print0 | xargs -0 -n1 dirname | sort -u | cut -c30- > $main/tmp/icon-tmp3
find -L /usr/share/harbour-themepack-* -type d -maxdepth 1 -iname "overlay" -print0 | xargs -0 -n1 dirname | sort -u | cut -c30- > $main/tmp/icon-tmp4
find -L /usr/share/harbour-themepack-* -type d -maxdepth 1 -iname "dyncal" -print0 | xargs -0 -n1 dirname | sort -u | cut -c30- > $main/tmp/icon-tmp5
find -L /usr/share/harbour-themepack-* -type d -maxdepth 1 -iname "dynclock" -print0 | xargs -0 -n1 dirname | sort -u | cut -c30- > $main/tmp/icon-tmp6
cat $main/tmp/icon-tmp* | sort | uniq > $main/tmp/icon.menu

while :
do
    clear
    cat<<EOF
 Theme pack support for Sailfish OS
 ==================================

 Please enter your choice:
 ----------------------------------
   (A)pply icon theme
   (R)estore
   Auto-(U)pdate icon theme
   (B)ack
 ----------------------------------
 Current icon pack: $(<$main/icon-current)

EOF
    read -n1 -s
    case "$REPLY" in
    "A"|"a")  $main/iconapply-menu.sh ;;
    "R"|"r")  echo "This will restore your default icon pack. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) $main/icon-restore.sh
		touch /usr/share/applications/*.desktop
		if [ -d /usr/share/sailfishos-uithemer ]; then
		dconf write /desktop/lipstick/sailfishos-uithemer/activeIconPack "'default'"
		fi
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
     "U"|"u")  $main/autoupdate-menu.sh ;;
     "B"|"b")  rm -r $main/tmp/*; exit ;;
     * )  echo "invalid option"; sleep 1 ;;
    esac
    sleep 1
done
