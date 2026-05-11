#!/bin/sh
# sailfishos-uithemer-rescan: oneshot helper for the rescan.path unit.
# Reads the user's last "apply overlay" choice from dconf so the daemon
# generates overlays for newly-installed apps the active pack has no
# asset for, then asks helperd to run a unified rescan over both
# /usr/share/applications and /home/defaultuser/.local/share/applications.
# Falls back to overlay=false when the dconf key is unset (fresh
# install, never applied a theme yet, or the user opted out).
v=$(dconf read /desktop/lipstick/sailfishos-uithemer/iconOverlay 2>/dev/null)
case "$v" in
    true)  o=true ;;
    *)     o=false ;;
esac
exec dbus-send --system --type=method_call --print-reply --reply-timeout=5000 \
    --dest=org.uithemer.UiThemer1 \
    /org/uithemer/UiThemer1 \
    org.uithemer.UiThemer1.Themes.ThemeNewDesktops "boolean:$o"
