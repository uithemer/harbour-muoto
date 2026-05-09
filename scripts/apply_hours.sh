#!/bin/bash

timer=$1
main=/usr/share/sailfishos-uithemer
echo $timer > $main/service/hours

    case "$timer" in
    "30")  echo '[Unit]
Description=Timer for updating icon theme via Theme pack support.
[Timer]
OnBootSec=0
OnCalendar=*-*-* *:0/30
Persistent=true
OnActiveSec=30m

[Install]
WantedBy=timers.target' > /etc/systemd/system/themepacksupport-autoupdate.timer ;;
    "1")  echo '[Unit]
Description=Timer for updating icon theme via Theme pack support.
[Timer]
OnBootSec=0
OnCalendar=hourly
Persistent=true
OnActiveSec=1h

[Install]
WantedBy=timers.target' > /etc/systemd/system/themepacksupport-autoupdate.timer ;;
    "2")  echo '[Unit]
Description=Timer for updating icon theme via Theme pack support.
[Timer]
OnBootSec=0
OnCalendar=2h
Persistent=true
OnActiveSec=2h

[Install]
WantedBy=timers.target' > /etc/systemd/system/themepacksupport-autoupdate.timer ;;
    "3")  echo '[Unit]
Description=Timer for updating icon theme via Theme pack support.
[Timer]
OnBootSec=0
OnCalendar=3h
Persistent=true
OnActiveSec=3h

[Install]
WantedBy=timers.target' > /etc/systemd/system/themepacksupport-autoupdate.timer ;;
    "6")  echo '[Unit]
Description=Timer for updating icon theme via Theme pack support.
[Timer]
OnBootSec=0
OnCalendar=6h
Persistent=true
OnActiveSec=6h

[Install]
WantedBy=timers.target' > /etc/systemd/system/themepacksupport-autoupdate.timer ;;
    "12")  echo '[Unit]
Description=Timer for updating icon theme via Theme pack support.
[Timer]
OnBootSec=0
OnCalendar=12h
Persistent=true
OnActiveSec=12h

[Install]
WantedBy=timers.target' > /etc/systemd/system/themepacksupport-autoupdate.timer ;;



    * )  echo '[Unit]
Description=Timer for updating icon theme via Theme pack support.
[Timer]
OnBootSec=0
OnCalendar='$timer'
Persistent=true
OnActiveSec=24h

[Install]
WantedBy=timers.target' > /etc/systemd/system/themepacksupport-autoupdate.timer ;;
		esac

source "$main/config.shlib"
if [[ "$(config_get autoupd)" == "1" ]]; then
    systemctl daemon-reload
else
    systemctl enable themepacksupport-autoupdate.timer
    systemctl start themepacksupport-autoupdate.timer
    systemctl enable themepacksupport-autoupdate.service
    systemctl start themepacksupport-autoupdate.service
    config_write autoupd 1
fi
