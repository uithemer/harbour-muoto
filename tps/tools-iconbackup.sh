#!/bin/bash

# Set directory variables
main=/usr/share/sailfishos-uithemer

name="uithemerbackup_"$(date +%Y%m%d%H%M%S)

echo "creating icons backup..."

if [ ! -d $main/backup/icons ]; then
	$main/icon-backup.sh
fi

cd $main/backup/
tar -zcf /home/nemo/$name.tar.gz ./icons
chown nemo:nemo /home/nemo/$name.tar.gz

echo "/home/nemo/$name.tar.gz created"
