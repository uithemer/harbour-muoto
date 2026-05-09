#!/bin/bash

ssu ar nieldk_obs_arm http://repo.merproject.org/obs/home:/nielnielsen/sailfish_latest_armv7hl/
ssu ar nieldk_obs_486 http://repo.merproject.org/obs/home:/nielnielsen/latest_i486/
pkcon repo-set-data nieldk_obs_arm refresh-now true
pkcon repo-set-data nieldk_obs_486 refresh-now true
pkcon install -y ImageMagick libMagickCore libMagickWand
ssu rr nieldk_obs_arm
ssu rr nieldk_obs_486

exit 0
