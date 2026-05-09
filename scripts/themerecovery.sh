#!/bin/bash

icons=$1
fonts=$2
sounds=$3

main=/usr/share/sailfishos-uithemer

if [ "$icons" = 1 ]; then
    echo "reinstalling icons"
    $main/icon-reinstall.sh
fi

if [ "$fonts" = 1 ]; then
    echo "reinstalling fonts"
    $main/font-reinstall.sh
fi

if [ "$sounds" = 1 ]; then
    echo "reinstalling sounds"
    $main/sound-reinstall.sh
fi
