#!/bin/bash

# Set directory variables
backup=/usr/share/sailfishos-uithemer/backup/dlocks
system=/etc/dconf/db/vendor.d/locks/

if [ -f $backup/silica-configs.txt.bk ]; then
	mv $backup/silica-configs.txt.bk $system/silica-configs.txt
fi

if [ -f $backup/ui-configs.txt.bk ]; then
	mv $backup/ui-configs.txt.bk $system/ui-configs.txt
fi

dconf update

exit 0
