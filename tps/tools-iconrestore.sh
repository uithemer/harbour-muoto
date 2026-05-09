#!/bin/bash

# Usage ./tools-iconbackup.sh [/path/backupname.tar.gz]

# Set backup variable
name=$1

# Is the variable empty?
if [ -z "$name" ]; then
    exit 1
fi

# Set directory variables
main=/usr/share/sailfishos-uithemer

echo "restoring icons backup..."

cd $main/backup
mkdir -p icons
tar -zxf --overwrite $name -C .

echo "$name restored"
