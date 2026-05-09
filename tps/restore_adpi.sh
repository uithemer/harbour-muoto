#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer

# Set build.prop position
device=$(<$main/device-model)
model=( "h3113" "h3123" "h3133" "h4113" "h4133" "h3413" "h4413" "h4493" "h3213" "h3223" "h4213" "h4233" )
if [[ " ${model[*]} " == *" $device "* ]]; then
	dir_droid=/vendor
else
	dir_droid=/opt/alien/system
fi

# Check if default Android DPI backup exists
if [ -f $main/backup/droiddpi ]; then
	# Restore Android DPI
	sed -i "s/.*ro.sf.lcd_density.*/ro.sf.lcd_density=$(<$main/backup/droiddpi)/" $dir_droid/build.prop
	echo "" > $main/droiddpi-current
	grep "^ro.sf.lcd_density" $dir_droid/build.prop | tr ":" " " | egrep -o '.{1,3}$' > $main/droiddpi-current
fi
