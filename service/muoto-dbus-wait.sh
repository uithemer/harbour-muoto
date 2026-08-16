#!/bin/sh
# Shared helpers for harbour-muoto shell scripts talking to org.muoto.Muoto1.
# shellcheck shell=sh

MUOTO_SERVICE=org.muoto.Muoto1
MUOTO_PATH=/org/muoto/Muoto1
MUOTO_LAUNCHER_SERVICE=org.muoto.Launcher1
MUOTO_LAUNCHER_PATH=/org/muoto/Launcher1
MUOTO_LAUNCHER_THEMES=org.muoto.Launcher1.Themes
OS_UPDATE_FLAG=/run/defaultuser/osupdate_running
MUOTO_BACKUP_ICONS=/usr/share/harbour-muoto/backup/icons
MUOTO_OP_STATUS=/usr/share/harbour-muoto/last-op.json

# stderr → journal when run under harbour-muoto-install-listener (ForwardedChannels).
muoto_log() {
    echo "muoto: $*" >&2
}

# SFOS: defaultuser runtime flag and/or upgrade systemd units (not /tmp/os-update-running).
muoto_os_update_running() {
    if [ -f "$OS_UPDATE_FLAG" ]; then
        muoto_log "os update: $OS_UPDATE_FLAG present"
        return 0
    fi
    for _u in system-update.target sailfish-upgrade-ui.service; do
        if systemctl is-active --quiet "$_u" 2>/dev/null; then
            muoto_log "os update: $_u active"
            return 0
        fi
    done
    return 1
}

# Stock icon backup tree (IconPaths::backupIconsRoot in C++):
#   $MUOTO_BACKUP_ICONS/jolla/<z>/icons/*.png
#   $MUOTO_BACKUP_ICONS/native/<size>/apps/*.png
#   $MUOTO_BACKUP_ICONS/apk/*.png
# Populated on ApplyIcons in 3.1 bulk era; cleared on migration to 3.2.
muoto_icons_backup_present() {
    [ -d "$MUOTO_BACKUP_ICONS" ] || return 1
    if find "$MUOTO_BACKUP_ICONS" -name '*.png' -print -quit 2>/dev/null | grep -q .; then
        return 0
    fi
    return 1
}

# Log when dconf says themed but backup tree has no PNGs (stale state).
muoto_warn_if_themed_without_backup() {
    pack=$(muoto_dconf_as_user "dconf read /apps/harbour-muoto/activeIconPack" 2>/dev/null || true)
    pack=${pack#\'}
    pack=${pack%\'}
    if [ -n "$pack" ] && [ "$pack" != "default" ]; then
        echo "muoto: activeIconPack='$pack' but no backup/icons PNGs; skipping RestoreIcons" >&2
    fi
}

# Session bus dbus-send as defaultuser (launcher-icond).
muoto_dbus_session_send() {
    _dest="$1"
    _path="$2"
    _iface_method="$3"
    shift 3
    _pref=$(muoto_dconf_env_prefix) || return 1
    if [ "$(id -un)" = "defaultuser" ]; then
        # shellcheck disable=SC2086
        sh -c "$_pref dbus-send --session --type=method_call \
            --dest=$_dest $_path $_iface_method $*"
        return $?
    fi
    # shellcheck disable=SC2086
    su defaultuser -c "$_pref dbus-send --session --type=method_call \
        --dest=$_dest $_path $_iface_method $*"
}

muoto_launcher_service_registered() {
    if [ "$(id -un)" = "defaultuser" ]; then
        dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus \
            org.freedesktop.DBus.GetNameOwner "string:$MUOTO_LAUNCHER_SERVICE" >/dev/null 2>&1
        return $?
    fi
    _pref=$(muoto_dconf_env_prefix) || return 1
    su defaultuser -c "$_pref dbus-send --session --print-reply \
        --dest=org.freedesktop.DBus /org/freedesktop/DBus \
        org.freedesktop.DBus.GetNameOwner string:$MUOTO_LAUNCHER_SERVICE" \
        >/dev/null 2>&1
}

muoto_ensure_launcher_icond() {
    if muoto_launcher_service_registered; then
        muoto_log "ensure_launcher_icond: $MUOTO_LAUNCHER_SERVICE on session bus"
        return 0
    fi
    muoto_log "ensure_launcher_icond: starting user unit"
    muoto_run_as_user 'systemctl --user start harbour-muoto-launcher-icond.service' || true
    i=0
    while [ "$i" -lt 5 ]; do
        if muoto_launcher_service_registered; then
            muoto_log "ensure_launcher_icond: ready (${i}s)"
            return 0
        fi
        sleep 1
        i=$((i + 1))
    done
    muoto_log "ensure_launcher_icond: systemctl failed, ensuring unit + binary"
    if ! muoto_run_as_user 'test -e ~/.config/systemd/user/harbour-muoto-launcher-icond.service'; then
        muoto_run_as_user 'mkdir -p ~/.config/systemd/user && ln -sf /usr/lib/systemd/user/harbour-muoto-launcher-icond.service ~/.config/systemd/user/' || true
        muoto_run_as_user 'systemctl --user daemon-reload && systemctl --user start harbour-muoto-launcher-icond.service' || true
    fi
    muoto_run_as_user 'nohup /usr/libexec/harbour-muoto-launcher-icond >/dev/null 2>&1 &' || true
    i=0
    while [ "$i" -lt 15 ]; do
        if muoto_launcher_service_registered; then
            muoto_log "ensure_launcher_icond: ready (${i}s)"
            return 0
        fi
        sleep 1
        i=$((i + 1))
    done
    muoto_log "ensure_launcher_icond: failed after 15s"
    return 1
}

# Start helperd before dbus-send. Root: systemctl (reliable during rpm %preun).
# Non-root: no systemctl (Polkit PIN). StartService is missing on some SFOS dbus;
# Introspect or the icon op dbus-send activates via org.muoto.Muoto1.service.
muoto_ensure_helperd() {
    _as_root=false
    if [ "$(id -u)" -eq 0 ]; then
        _as_root=true
        muoto_log "ensure_helperd: systemctl start (uid=0)"
        systemctl start harbour-muoto-helperd.service 2>/dev/null || true
    else
        if dbus-send --system --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus \
            org.freedesktop.DBus.GetNameOwner "string:$MUOTO_SERVICE" >/dev/null 2>&1; then
            muoto_log "ensure_helperd: $MUOTO_SERVICE already on bus"
            return 0
        fi
        muoto_log "ensure_helperd: dbus activate (uid=$(id -u), no systemctl)"
        dbus-send --system --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus \
            org.freedesktop.DBus.StartService "string:$MUOTO_SERVICE" >/dev/null 2>&1 || true
        dbus-send --system --type=method_call --dest="$MUOTO_SERVICE" "$MUOTO_PATH" \
            org.freedesktop.DBus.Introspectable.Introspect >/dev/null 2>&1 || true
    fi
    i=0
    while [ "$i" -lt 15 ]; do
        if dbus-send --system --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus \
            org.freedesktop.DBus.GetNameOwner "string:$MUOTO_SERVICE" >/dev/null 2>&1; then
            muoto_log "ensure_helperd: $MUOTO_SERVICE ready (${i}s)"
            return 0
        fi
        sleep 1
        i=$((i + 1))
    done
    if [ "$_as_root" = true ]; then
        muoto_log "ensure_helperd: failed after 15s (root)"
        return 1
    fi
    muoto_log "ensure_helperd: not on bus after 15s; icon dbus-send may activate"
    return 0
}

muoto_wait_op_cancel() {
    if [ -n "${_WAIT_FD:-}" ]; then
        eval "exec ${_WAIT_FD}>&-" 2>/dev/null || true
        _WAIT_FD=
    fi
}

# Set expected op and flock timeout (seconds) before dbus-send.
# Uninstall restore uses 60; boot apply and pre-upgrade restore use 180.
# Usage: muoto_wait_op_begin RestoreIcons 60
#        dbus-send ...
#        muoto_wait_op_end RestoreIcons
_WAIT_OP=
_WAIT_TIMEOUT=

_muoto_wait_flock() {
    _lock=/usr/share/harbour-muoto/icon-ops.lock
    touch "$_lock" 2>/dev/null || true
    # shellcheck disable=SC3028
    exec 200>"$_lock"
    _WAIT_FD=200
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
            muoto_wait_op_cancel
            return 0
        fi
        sleep 1
        i=$((i + 1))
    done
    echo "muoto: timed out waiting for icon op lock (${_WAIT_OP:-?})" >&2
    muoto_wait_op_cancel
    return 1
}

# Last operation outcome, written by launcher-icond next to the manifest.
# The lock lifecycle only tells us an operation ran, never whether it did
# anything -- which is why a rejected apply used to be logged as a success.
# Reading OperationCompleted from a script would mean dbus-monitor; this is a
# file with a monotonic sequence instead.
muoto_op_status_sequence() {
    [ -r "$MUOTO_OP_STATUS" ] || { echo 0; return 0; }
    _seq=$(sed -n 's/.*"sequence":\([0-9][0-9]*\).*/\1/p' "$MUOTO_OP_STATUS" 2>/dev/null)
    echo "${_seq:-0}"
}

muoto_op_status_outcome() {
    [ -r "$MUOTO_OP_STATUS" ] || { echo unknown; return 0; }
    _out=$(sed -n 's/.*"outcome":"\([a-z][a-z]*\)".*/\1/p' "$MUOTO_OP_STATUS" 2>/dev/null)
    echo "${_out:-unknown}"
}

muoto_wait_op_begin() {
    _WAIT_OP="$1"
    _WAIT_TIMEOUT="${2:-180}"
}

muoto_wait_op_end() {
    expected="${1:-$_WAIT_OP}"
    if [ -n "$expected" ] && [ "$expected" != "${_WAIT_OP:-}" ]; then
        echo "muoto: wait_op_end expected '$expected' but began '$_WAIT_OP'" >&2
        return 1
    fi
    _muoto_wait_flock
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
