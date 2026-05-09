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
   Install required (D)ependencies
   Install Image(M)agick
   Back(U)p icons
   (R)estore icons backup
   (O)ne-click restore service
   Reinstall (I)cons
   Reinstall (F)onts
   Reinstall (S)ounds
   Unapply (A)ll Patchmanager patches
   Uninstall (P)atchmanager
   Reinstall (L)ipstick
   (B)ack
 ----------------------------------

EOF
    read -n1 -s
    case "$REPLY" in
    "D"|"d")  echo "Install required dependencies? y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) 	echo "It may take some time, do not quit."
		$main/install_dependencies.sh; echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "M"|"m")  echo "Install ImageMagick? y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) 	echo "It may take some time, do not quit."
		$main/install_imagemagick.sh; echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "U"|"u")  echo "Backup icons? y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) 	$main/tools-iconbackup.sh; echo "done!"; sleep 3 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "R"|"r")  read -p "Please enter the full path of the backup file or 'q' to exit and press enter: " choice
		case "$choice" in 
		q|Q ) echo "ok"; sleep 1;;
		* ) $main/tools-iconrestore.sh $choice; echo "done!"; sleep 3 ;;
		esac ;;
    "O"|"o")  $main/systemupgrade-service.sh ;;
    "I"|"i")  echo "This will restore the default Sailfish OS icons. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) $main/icon-reinstall.sh
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "F"|"f")  echo "This will restore the default Sailfish OS fonts. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) $main/font-reinstall.sh
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "S"|"s")  echo "This will restore the default Sailfish OS sounds. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) $main/sound-reinstall.sh
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "A"|"a")  echo "This will unapply all Patchmanager patches. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) patchmanager --unapply-all
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "P"|"p")  echo "This will uninstall Patchmanager and restore the Sailfish OS UI. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) patchmanager --unapply-all
		pkcon remove ausmt
		pkcon install --allow-reinstall -y lipstick-qt5 lipstick-jolla-home-qt5 jolla-settings jolla-settings-system
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "L"|"l")  echo "This will restore the Sailfish OS UI. Continue y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) pkcon  install --allow-reinstall -y lipstick-qt5 lipstick-jolla-home-qt5 jolla-settings jolla-settings-system
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
     "B"|"b")  exit                      ;;
     * )  echo "invalid option"; sleep 1     ;;
    esac
    sleep 1
done
