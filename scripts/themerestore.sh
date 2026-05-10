#!/bin/bash

fonts=$1
sounds=$2

main=/usr/share/sailfishos-uithemer

if [ "$fonts" = 1 ]; then
    echo "restoring fonts"
    $main/font-restore.sh
fi

if [ "$sounds" = 1 ]; then
    echo "restoring sounds"
    $main/sound-restore.sh
fi
