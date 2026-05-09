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
   (E)nable one-click restore
    before system upgrades
   (D)isable one-click restore
    before system upgrades
   (B)ack
 ----------------------------------

EOF
    read -n1 -s
    case "$REPLY" in
    "E"|"e")  echo "Enable one-click restore before system upgrades y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) systemctl enable themepacksupport-systemupgrade.service
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "D"|"d")  echo "Disable one-click restore before system upgrades y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) systemctl disable themepacksupport-systemupgrade.service
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
     "B"|"b")  exit                      ;;
     * )  echo "invalid option"; sleep 1     ;;
    esac
    sleep 1
done
