#!/bin/bash

echo "disabling ocr before system upgrades..."

systemctl disable themepacksupport-systemupgrade.service

echo "done"

exit 0
