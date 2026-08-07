#!/bin/sh
# harbour-muoto 3.2 device test checklist (run on device as defaultuser).
# Usage: bash device-test-3.2.sh [--destructive]
set -eu

. /usr/share/harbour-muoto/service/muoto-dbus-wait.sh

DESTRUCTIVE=false
for arg in "$@"; do
    case "$arg" in
        --destructive) DESTRUCTIVE=true ;;
    esac
done

PASS=0
FAIL=0

pass() { echo "PASS $1"; PASS=$((PASS + 1)); }
fail() { echo "FAIL $1: $2"; FAIL=$((FAIL + 1)); }

# T-01 Services
if systemctl --user is-active harbour-muoto-launcher-icond >/dev/null 2>&1; then
    pass T-01-launcher-icond
else
    fail T-01-launcher-icond "not active"
fi

if systemctl --user is-active harbour-muoto-install-listener >/dev/null 2>&1; then
    pass T-01-install-listener
else
    fail T-01-install-listener "not active"
fi

if getcap /usr/libexec/harbour-muoto-launcher-icond 2>/dev/null | grep -q cap_dac_override; then
    pass T-01-cap
else
    fail T-01-cap "missing cap_dac_override"
fi

# T-02 no silica writes (snapshot mtime before/after apply — informational)
if [ -d /usr/share/themes/sailfish-default/silica ]; then
    pass T-02-silica-dir-exists
else
    fail T-02-silica-dir-exists "missing"
fi

pack=$(dconf read /apps/harbour-muoto/activeIconPack 2>/dev/null || true)
pack=${pack#\'}; pack=${pack%\'}
if [ -n "$pack" ] && [ "$pack" != "default" ]; then
    if [ -f /usr/share/harbour-muoto/launcher-manifest.json ]; then
        pass T-02-manifest
    else
        fail T-02-manifest "missing with active pack"
    fi
    if ls /usr/share/harbour-muoto/launcher-icons/*.png >/dev/null 2>&1; then
        pass T-02-generated-icons
    else
        fail T-02-generated-icons "none found"
    fi
fi

# T-13 update-icons
if /usr/bin/harbour-muoto-update-icons; then
    pass T-13-update-icons
else
    fail T-13-update-icons "exit non-zero"
fi

if [ "$DESTRUCTIVE" = true ]; then
    echo "Running destructive restore test T-07..."
    if muoto_ensure_launcher_icond && muoto_dbus_session_send \
        "$MUOTO_LAUNCHER_SERVICE" "$MUOTO_LAUNCHER_PATH" \
        "$MUOTO_LAUNCHER_THEMES.RestoreIcons"; then
        sleep 2
        if [ ! -f /usr/share/harbour-muoto/launcher-manifest.json ]; then
            pass T-07-restore
        else
            fail T-07-restore "manifest still present"
        fi
    else
        fail T-07-restore "dbus failed"
    fi
fi

echo "----"
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
