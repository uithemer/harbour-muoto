#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer
system=/etc/dconf/db/vendor.d/locks/

# Set build.prop position
device=$(<$main/device-model)
model=( "h3113" "h3123" "h3133" "h4113" "h4133" "h3413" "h4413" "h4493" "h3213" "h3223" "h4213" "h4233" )
if [[ " ${model[*]} " == *" $device "* ]]; then
	dir_droid=/vendor
else
	dir_droid=/opt/alien/system
fi

mkdir -p $main/tmp
mkdir -p $main/backup
mkdir -p $main/backup/dlocks

if [ -f $system/silica-configs.txt ]; then
	mv $system/silica-configs.txt $main/backup/dlocks/silica-configs.txt.bk
fi

if [ -f $system/ui-configs.txt ]; then
	mv $system/ui-configs.txt $main/backup/dlocks/ui-configs.txt.bk
fi

dconf update

dconf read /desktop/sailfish/silica/icon_size_launcher | egrep -o '[0-9].[0-9]+' > $main/icon-z

if [ -f $dir_droid/build.prop ]; then
	grep "^ro.sf.lcd_density" $dir_droid/build.prop | tr ":" " " | egrep -o '.{1,3}$' > $main/droiddpi-current
fi

exit 0
