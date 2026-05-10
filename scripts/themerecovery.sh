#!/bin/bash

fonts=$1
sounds=$2

main=/usr/share/sailfishos-uithemer

if [ "$fonts" = 1 ]; then
    echo "reinstalling fonts"
    $main/font-reinstall.sh
fi

if [ "$sounds" = 1 ]; then
    echo "reinstalling sounds"
    $main/sound-reinstall.sh
fi
