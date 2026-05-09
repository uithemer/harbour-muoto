#!/bin/bash

# Make sure only root can run the script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root. Please gain root privileges and try again." 1>&2
   exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer

while :
do
    clear
    cat<<EOF
 Theme pack support for Sailfish OS
 ==================================

 Please enter your choice:
 ----------------------------------
   (E)nable display density 
   settings
   (D)isable display density 
   settings
   (A)pply device pixel ratio
   (R)estore device pixel ratio
   (S)et icon size
   (C)hange Alien Dalvik DPI
   Restore Alien Dalvik (D)PI
   (B)ack
 ----------------------------------

EOF
    read -n1 -s
    case "$REPLY" in
    "E"|"e")    $main/enable-dpi.sh
		if [ -d /usr/share/sailfishos-uithemer ]; then
		dconf write /desktop/lipstick/sailfishos-uithemer/densityEnabled true
		fi
		echo "done!"; sleep 1 ;;
    "D"|"d")    $main/disable-dpi.sh
		if [ -d /usr/share/sailfishos-uithemer ]; then
		dconf write /desktop/lipstick/sailfishos-uithemer/densityEnabled false
		fi
		echo "done!"; sleep 1 ;;
    "A"|"a")  	read -p "Please enter the device pixel ratio or 'q' to exit and press enter: " choice
		case "$choice" in 
		q|Q ) echo "ok"; sleep 1;;
		* )  dconf write /desktop/sailfish/silica/theme_pixel_ratio $choice
		echo "done!"; sleep 1 ;;
		esac ;;
    "R"|"r")  echo "This will restore your default device pixel ratio. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) dconf reset /desktop/sailfish/silica/theme_pixel_ratio
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "S"|"s")  	read -p "Please enter the preferred icon size (choose between 86, 108, 129, 151, 172) or 'q' to exit and press enter: " choice
		case "$choice" in 
		q|Q ) echo "ok"; sleep 1;;
		* ) dconf write /desktop/sailfish/silica/icon_size_launcher $choice
		echo "done!"; sleep 1 ;;
		esac ;;
    "C"|"c")  read -p "Please enter DPI value for Alien Dalvik or 'q' to exit and press enter: " aliensize
		case "$aliensize" in 
		[0-3]* ) $main/apply_adpi.sh $aliensize
		echo "done!"; sleep 1 ;;
		q|Q ) echo "quit"; sleep 1 ;;
		esac ;;
    "D"|"d")  echo "This will restore your default DPI in Alien Dalvik. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) $main/restore_adpi.sh
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
     "B"|"b")  exit                      ;;
     * )  echo "invalid option"; sleep 1     ;;
    esac
    sleep 1
done
