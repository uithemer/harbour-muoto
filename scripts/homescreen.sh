#!/bin/bash

# Restart the user-session lipstick (homescreen).
#
# Why this is more than `systemctl-user restart lipstick.service`:
#
# When the GUI binary calls this script, it has just done setuid(0) — so we
# are running as root, with root's empty DBus session and no XDG_RUNTIME_DIR.
# `systemctl --user` then talks to ROOT's user manager, which has no
# lipstick.service loaded, and the session bus connection fails with
# "Failed to connect to bus: Operation not permitted".
#
# So when we are root, drop to defaultuser with the right session env and
# then ask its user systemd to restart lipstick. If we are already running
# as the desktop user (CLI path), just run systemctl --user directly.

echo "restarting homescreen..."

target_user=defaultuser
target_uid=$(id -u "$target_user" 2>/dev/null)

if [ -z "$target_uid" ]; then
    echo "homescreen.sh: user '$target_user' not found" >&2
    exit 1
fi

runtime_dir="/run/user/$target_uid"
bus_addr="unix:path=${runtime_dir}/bus"

run_user_systemctl() {
    XDG_RUNTIME_DIR="$runtime_dir" \
    DBUS_SESSION_BUS_ADDRESS="$bus_addr" \
        systemctl --user restart lipstick.service
}

if [ "$(id -u)" -eq 0 ] && [ "$target_user" != "root" ]; then
    # su is the most portable way on SFOS; runuser is not always present.
    su -s /bin/sh "$target_user" -c "
        XDG_RUNTIME_DIR='$runtime_dir' \
        DBUS_SESSION_BUS_ADDRESS='$bus_addr' \
        systemctl --user restart lipstick.service
    "
else
    run_user_systemctl
fi
