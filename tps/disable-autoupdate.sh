#!/bin/bash

main=/usr/share/sailfishos-uithemer
source $main/config.shlib

systemctl disable themepacksupport-autoupdate.timer
systemctl stop themepacksupport-autoupdate.timer
systemctl disable themepacksupport-autoupdate.service
systemctl stop themepacksupport-autoupdate.service
config_write autoupd 0
