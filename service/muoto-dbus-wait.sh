#!/bin/sh
# Shared helpers for harbour-muoto shell scripts talking to org.muoto.Muoto1.
# shellcheck shell=sh

MUOTO_SERVICE=org.muoto.Muoto1
MUOTO_PATH=/org/muoto/Muoto1
MUOTO_THEMES=org.muoto.Muoto1.Themes
OS_UPDATE_SENTINEL=/tmp/os-update-running

muoto_os_update_running() {
    [ -f "$OS_UPDATE_SENTINEL" ]
}

# Start dbus-monitor for OperationCompleted; call before dbus-send.
# Usage: muoto_wait_op_begin ApplyIcons 180
#        dbus-send ...
#        muoto_wait_op_end ApplyIcons -> exit 0 on success, 1 on failure
_WAIT_TMP=
_WAIT_MON_PID=
_WAIT_OP=
_WAIT_TIMEOUT=

# Parse dbus-monitor lines: signal header -> op string -> boolean -> message.
_dbus_parse_monitor_line() {
    line="$1"
    case "$line" in
        *member=OperationCompleted*)
            _dbus_state=1
            _dbus_got_op=
            return 0
            ;;
    esac

    case "${_dbus_state:-0}" in
        1)
            case "$line" in
                *string*)
                    _dbus_got_op=$(printf '%s\n' "$line" | sed -n 's/.*string "\([^"]*\)".*/\1/p')
                    _dbus_state=2
                    ;;
            esac
            ;;
        2)
            case "$line" in
                *boolean*true*)
                    if [ "$_dbus_got_op" = "$_WAIT_OP" ]; then
                        echo ok >"$_WAIT_TMP"
                        exit 0
                    fi
                    _dbus_state=0
                    ;;
                *boolean*false*)
                    if [ "$_dbus_got_op" = "$_WAIT_OP" ]; then
                        echo fail >"$_WAIT_TMP"
                        exit 1
                    fi
                    _dbus_state=0
                    ;;
            esac
            ;;
    esac
}

# defaultuser cannot BecomeMonitor on the system bus; wait on icon-ops.lock instead.
_muoto_wait_flock() {
    _lock=/usr/share/harbour-muoto/icon-ops.lock
    touch "$_lock" 2>/dev/null || true
    # shellcheck disable=SC3028
    exec 200>"$_lock"
    held=0
    i=0
    while [ "$i" -lt 15 ]; do
        if flock -n 200; then
            flock -u 200
            sleep 1
        else
            held=1
            break
        fi
        i=$((i + 1))
    done
    if [ "$held" != 1 ]; then
        echo "muoto: operation did not start (busy or rejected)" >&2
        return 1
    fi
    i=0
    while [ "$i" -lt "${_WAIT_TIMEOUT:-180}" ]; do
        if flock -n 200; then
            flock -u 200
            return 0
        fi
        sleep 1
        i=$((i + 1))
    done
    echo "muoto: timed out waiting for icon op lock ($_WAIT_OP)" >&2
    return 1
}

muoto_wait_op_begin() {
    _WAIT_OP="$1"
    _WAIT_TIMEOUT="${2:-180}"
    if [ "$(id -un)" = "defaultuser" ]; then
        return 0
    fi
    _WAIT_TMP=$(mktemp)
    (
        dbus-monitor --system \
            "type='signal',path='${MUOTO_PATH}',interface='${MUOTO_THEMES}',member='OperationCompleted'" \
        | while read -r line; do
            _dbus_parse_monitor_line "$line"
        done
    ) &
    _WAIT_MON_PID=$!
    sleep 0.3
}

muoto_wait_op_end() {
    expected="${1:-$_WAIT_OP}"
    if [ "$(id -un)" = "defaultuser" ]; then
        _muoto_wait_flock
        return $?
    fi
    i=0
    while [ "$i" -lt "${_WAIT_TIMEOUT:-180}" ]; do
        if [ -f "$_WAIT_TMP" ]; then
            res=$(cat "$_WAIT_TMP" 2>/dev/null) || res=
            kill "$_WAIT_MON_PID" 2>/dev/null || true
            wait "$_WAIT_MON_PID" 2>/dev/null || true
            rm -f "$_WAIT_TMP" 2>/dev/null || true
            case "$res" in
                ok) return 0 ;;
                fail)
                    echo "muoto: OperationCompleted($expected) failed" >&2
                    return 1
                    ;;
            esac
        fi
        sleep 1
        i=$((i + 1))
    done
    kill "$_WAIT_MON_PID" 2>/dev/null || true
    wait "$_WAIT_MON_PID" 2>/dev/null || true
    rm -f "$_WAIT_TMP" 2>/dev/null || true
    echo "muoto: timed out waiting for OperationCompleted($expected)" >&2
    return 1
}

# defaultuser session bus (SFOS: …/dbus/user_bus_socket, not …/bus).
muoto_defaultuser_uid() {
    id -u defaultuser 2>/dev/null || echo 100000
}

muoto_user_env_prefix() {
    _uid=$(muoto_defaultuser_uid)
    _rdir="/run/user/$_uid"
    if [ ! -d "$_rdir" ]; then
        return 1
    fi
    printf 'XDG_RUNTIME_DIR=%s HOME=/home/defaultuser' "$_rdir"
    return 0
}

# dconf under su(1) from root must use the user session bus (SFOS: …/dbus/user_bus_socket).
muoto_dconf_env_prefix() {
    _pref=$(muoto_user_env_prefix) || return 1
    _rdir="/run/user/$(muoto_defaultuser_uid)"
    if [ -S "$_rdir/dbus/user_bus_socket" ]; then
        _pref="$_pref DBUS_SESSION_BUS_ADDRESS=unix:path=$_rdir/dbus/user_bus_socket"
    fi
    printf '%s' "$_pref"
    return 0
}

muoto_run_as_user() {
    if [ "$(id -un)" = "defaultuser" ]; then
        if [ -z "${XDG_RUNTIME_DIR:-}" ]; then
            export XDG_RUNTIME_DIR="/run/user/$(id -u)"
        fi
        export HOME="${HOME:-/home/defaultuser}"
        sh -c "$1"
    else
        _pref=$(muoto_user_env_prefix) || {
            echo "muoto: no defaultuser session (missing /run/user/<uid>); cannot run: $1" >&2
            return 1
        }
        su defaultuser -c "$_pref $1"
    fi
}

muoto_run_as_user_or_die() {
    if ! muoto_run_as_user "$1"; then
        echo "muoto: command failed as defaultuser: $1" >&2
        exit 1
    fi
}

muoto_dconf_as_user() {
    if [ "$(id -un)" = "defaultuser" ]; then
        if [ -z "${XDG_RUNTIME_DIR:-}" ]; then
            export XDG_RUNTIME_DIR="/run/user/$(id -u)"
        fi
        export HOME="${HOME:-/home/defaultuser}"
        sh -c "$1"
    else
        _pref=$(muoto_dconf_env_prefix) || {
            echo "muoto: no defaultuser session (missing /run/user/<uid>); cannot run: $1" >&2
            return 1
        }
        su defaultuser -c "$_pref $1"
    fi
}

muoto_dconf_as_user_or_die() {
    if ! muoto_dconf_as_user "$1"; then
        echo "muoto: command failed as defaultuser: $1" >&2
        exit 1
    fi
}
