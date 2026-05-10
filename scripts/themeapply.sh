#!/bin/bash

theme=$1
font=$2
weight=$3
sound=$4
main=/usr/share/sailfishos-uithemer
source $main/config.shlib

function font-changer {
    if [ -d /usr/share/harbour-themepack-$theme/font ]; then
            $main/font-run.sh -f $theme -s $weight
    fi
    # If Android support is installed
    if [ -d /opt/alien/system/fonts ]; then
        if [ -d /usr/share/harbour-themepack-$theme/font ]; then
            if [ -s /usr/share/harbour-themepack-$theme/font/Regular.ttf ]; then
                if [ -s /usr/share/harbour-themepack-$theme/font/Light.ttf ]; then
                    $main/font-run.sh -f $theme -a Regular -d Light
                else
                    $main/font-run.sh -f $theme -a Regular -d Regular
                fi
            elif [ -s /usr/share/harbour-themepack-$theme/font/Light.ttf ]; then
                $main/font-run.sh -f $theme -a Light -d Light
            else
                echo "No fonts suitable for Android found"
            fi
        fi
    fi
}

if [ "$font" = 1 ]; then
    echo "applying font" $theme $weight
    $main/font-restore.sh
    $main/font-backup.sh
    font-changer
fi

if [ "$sound" = 1 ]; then
    echo "applying sounds" $theme
    $main/sound-restore.sh
    $main/sound-backup.sh
    $main/sound-run.sh $theme
fi
