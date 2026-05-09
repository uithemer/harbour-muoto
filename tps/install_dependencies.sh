#!/bin/bash

pkcon repo-set-data jolla refresh-now true
pkcon install --allow-reinstall -y sailfish-content-graphics-default-z1.0-base sailfish-content-graphics-default-z1.25-base sailfish-content-graphics-default-z1.5-base sailfish-content-graphics-default-z1.75-base sailfish-content-graphics-default-z2.0-base sailfish-content-graphics-closed-z1.0 sailfish-content-graphics-closed-z1.25 sailfish-content-graphics-closed-z1.5 sailfish-content-graphics-closed-z1.75 sailfish-content-graphics-closed-z2.0

exit 0
