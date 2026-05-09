#!/bin/bash

main=/usr/share/sailfishos-uithemer
iconpack=$(<$main/icon-current)
pack=/usr/share/harbour-themepack-$iconpack
source $main/config.shlib

echo "reapply icons" $iconpack

$main/icon-restore.sh
$main/icon-backup.sh
$main/icon-run.sh $iconpack

if [[ "$(config_get iconoverlay)" == "1" ]]; then
if [[ -d $pack/overlay && "$(ls $pack/overlay)" ]]; then
    echo "apply overlay"
    $main/icon-overlay.sh $iconpack
fi
fi

touch /usr/share/applications/*.desktop
