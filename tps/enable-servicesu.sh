#!/bin/bash

echo "enabling ocr before system upgrades..."

systemctl enable themepacksupport-systemupgrade.service

echo "done"

exit 0
