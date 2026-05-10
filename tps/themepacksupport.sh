#!/bin/bash
#
#    Theme pack support for Sailfish OS - Enables theme pack support in Sailfish OS.
#    Copyright (C) 2015-2018  fravaccaro <fravaccaro90@gmail.com>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Make sure only root can run the script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root. Please gain root privileges and try again." 1>&2
   exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer

# Load config file (key=value, sourced as shell assignments)
source $main/config.cfg

# Is logging set?
# if [ "$tps_log" == "1" ]; then
#	set -x
# elseif [ "$tps_log" == "0" ]; then
#	set +x
# fi

while :
do
    clear
    cat<<EOF
 Theme pack support for Sailfish OS
 ==================================

 Please enter your choice:
 ----------------------------------
   (L)ipstick refresh
   (O)ne-click restore
   (H)elp / UI Themer info
   (U)ninstall themes
   (M)anual
   (A)bout
   (Q)uit
 ----------------------------------
EOF
    read -n1 -s
    case "$REPLY" in
    "L"|"l")  echo "Refresh the homescreen? y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) 	echo "Your homescreen will be restarted..."
		systemctl-user restart lipstick.service; echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "M"|"m")  clear
cat<<EOF
 Theme pack support for Sailfish OS
 ==================================

Usage:

1. Install a compatible theme pack.
2. Apply the theme pack of your choice.
3. Refresh the homescreen.
4. To revert back to defaults use 'Restore' options.

EOF

		read -n1 -r -p "Press any key to continue..." ;;
    "A"|"a")  clear

#        \`///oo\`   :
#      /oyooyys\` -o/
#    :ssssoy+ossys+-
#   /oyosssosyo.\`
#  .sy+oysos:
#  +/sho+sh
#      o+sh
#      \`yoos:
#       \`+-+syo////.
#            \`-    :o-
#                    +.
#
		cat<<EOF
 Theme pack support for Sailfish OS
 ==================================

                  */
              .(*,#
            ,  **
           (%*  ,((.       ,*(
         *#.#,//   .*&%%/.
         (* #(.  ,(/*
         (,  ((/(,
         ,(,   ,(,
           *(   ,(
            %#, ,(
           .(/# #/
          .(.%%/
         ((*%*
     ,#&%/,

With Theme pack support you can customize icons, fonts and pixel density in Sailfish OS. Remember to unapply themes before system updates.
It can be also used via SSH to manage and restore Sailfish OS UI if anything goes wrong. Released under GPLv3.
The UI Themer app is included in the same package; launch it from the app grid.

E-mail: fravaccaro90@gmail.com 
Twitter: @itsamefra

EOF

		read -n1 -r -p "Press any key to continue..." ;;
    "O"|"o")  echo "Customizations must be reverted before performing a system update. With One-click restore you can automate this process and restore icons, fonts and display density settings with just one click. Continue? y/N? "
		read -n1 -s choice
		case "$choice" in 
		y|Y ) 	$main/ocr.sh
		echo "done!"; sleep 1 ;;
		* ) echo "aborted"; sleep 1 ;;
		esac ;;
    "H"|"h")  clear
		cat<<EOF
 UI Themer
 ==========

 UI Themer (sailfishos-uithemer) is installed together with these scripts.
 Open it from the application grid to use the graphical interface.

EOF
		read -n1 -r -p "Press any key to continue..." ;;

#    "L"|"l")  echo "Enable logs? y/N? "
#		read -n1 -s choice
#		case "$choice" in 
#		y|Y ) 	echo "Your homescreen will be restarted..."
#		systemctl-user restart lipstick.service; echo "done!"; sleep 1 ;;
#		* ) echo "aborted"; sleep 1 ;;
#		esac ;;
    "U"|"u")  ls -d /usr/share/harbour-themepack-* | sort -u | cut -c30-
		echo " "
		read -p "Please enter the theme pack name you want to uninstall or 'q' to exit and press enter: " choice
		case "$choice" in
		q|Q ) echo "ok"; sleep 1 ;;
		* ) # Get package name and remove it
		pkcon remove $(rpm -qf /usr/share/harbour-themepack-$choice/ --queryformat '%{NAME}\n') ;;
		esac ;;
    "Q"|"q")  rm -r $main/tmp/*; clear; exit                      ;;
     * )  echo "invalid option"; sleep 1     ;;
    esac
    sleep 1
done
