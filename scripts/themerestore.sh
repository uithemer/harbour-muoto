#!/bin/bash

fonts=$1

main=/usr/share/sailfishos-uithemer

if [ "$fonts" = 1 ]; then
    echo "restoring fonts"
    $main/font-restore.sh
fi
