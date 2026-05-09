#!/bin/bash

dpr=$1
adpi=$2

main=/usr/share/sailfishos-uithemer

if [ "$dpr" = 1 ]; then
    echo "restoring dpr"
    $main/restore_dpr.sh
    sleep 1
fi

if [ -d /opt/alien ]; then
    if [ "$adpi" = 1 ]; then
        echo "restoring android dpi"
        $main/restore_adpi.sh
    fi
fi

sleep 1
