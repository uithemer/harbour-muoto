#!/bin/bash

# Compatibility shim for the timer-driven autoupdate service.
# Icon refresh is now handled directly by the helper binary.

exec /usr/bin/sailfishos-uithemer-reassert --reassert
