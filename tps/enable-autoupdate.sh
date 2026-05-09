#!/bin/bash

main=/usr/share/sailfishos-uithemer
source $main/config.shlib

systemctl enable themepacksupport-autoupdate.timer
systemctl start themepacksupport-autoupdate.timer
systemctl enable themepacksupport-autoupdate.service
systemctl start themepacksupport-autoupdate.service
config_write autoupd 1
