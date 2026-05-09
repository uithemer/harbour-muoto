#!/bin/bash

icons=$1
fonts=$2
sounds=$3

main=/usr/share/sailfishos-uithemer

if [ "$icons" = 1 ]; then
    echo "restoring icons"
    $main/icon-restore.sh
    touch /usr/share/applications/*.desktop
fi

if [ "$fonts" = 1 ]; then
    echo "restoring fonts"
    $main/font-restore.sh
fi

if [ "$sounds" = 1 ]; then
    echo "restoring sounds"
    $main/sound-restore.sh
fi
