#!/bin/bash

dpr=$1

main=/usr/share/sailfishos-uithemer

if [ "$dpr" = 1 ]; then
    echo "restoring dpr"
    $main/restore_dpr.sh
    sleep 1
fi

sleep 1
