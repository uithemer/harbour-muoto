#!/bin/bash

fonts=$1

main=/usr/share/sailfishos-uithemer

if [ "$fonts" = 1 ]; then
    echo "reinstalling fonts"
    $main/font-reinstall.sh
fi
