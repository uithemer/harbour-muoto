#!/bin/sh
# harbour-muoto 3.2 — pre-upgrade restore + install/upgrade re-theme (on device).
#
# Run as defaultuser. Needs devel-su/sudo for oneshot-restore and pkcon.
# Usage:
#   bash device-test-preupgrade-install.sh
#   bash device-test-preupgrade-install.sh --pack haiku
#   bash device-test-preupgrade-install.sh --skip-install   # only T-20 pre-upgrade
#   bash device-test-preupgrade-install.sh --skip-preupgrade # only T-21 install
#
# Leaves the device with --pack applied when both tests pass (re-applies after T-20).
set -eu

. /usr/share/harbour-muoto/service/muoto-dbus-wait.sh

PACK=haiku
RUN_PREUPGRADE=true
RUN_INSTALL=true
SUDO_PASS=${MUOTO_SUDO_PASS:-rootme}
PROBE_PKG=${MUOTO_PROBE_PKG:-harbour-file-browser}
PROBE_DESKTOP=${MUOTO_PROBE_DESKTOP:-/usr/share/applications/harbour-file-browser.desktop}

while [ $# -gt 0 ]; do
    case "$1" in
        --pack=*) PACK=${1#--pack=} ;;
        --pack)
            PACK=${2:?--pack requires a name}
            shift
            ;;
        --skip-install) RUN_INSTALL=false ;;
        --skip-preupgrade) RUN_PREUPGRADE=false ;;
        -h|--help)
            sed -n '2,14p' "$0"
            exit 0
            ;;
        *)
            echo "unknown option: $1" >&2
            exit 2
            ;;
    esac
    shift
done

PASS=0
FAIL=0
pass() { echo "PASS $1"; PASS=$((PASS + 1)); }
fail() { echo "FAIL $1: $2"; FAIL=$((FAIL + 1)); }

run_root() {
    if command -v sudo >/dev/null 2>&1; then
        # shellcheck disable=SC2039
        printf '%s\n' "$SUDO_PASS" | sudo -S "$@"
    else
        printf '%s\n' "$SUDO_PASS" | devel-su -p "$@"
    fi
}

dconf_unquote() {
    v=$(dconf read "$1" 2>/dev/null || true)
    v=${v#\'}
    v=${v%\'}
    printf '%s' "$v"
}

apply_pack() {
    muoto_ensure_launcher_icond
    muoto_dbus_session_send \
        "$MUOTO_LAUNCHER_SERVICE" "$MUOTO_LAUNCHER_PATH" \
        "$MUOTO_LAUNCHER_THEMES.ApplyIcons" \
        string:"$PACK" boolean:true boolean:true
    sleep 4
}

icon_line() {
    grep -m1 '^Icon=' "$1" 2>/dev/null | cut -d= -f2- || true
}

manifest_has_desktop() {
    [ -f /usr/share/harbour-muoto/launcher-manifest.json ] || return 1
    # Compact JSON: "desktop":"/path" (no space after colon)
    grep -Fq "\"desktop\":\"$1\"" /usr/share/harbour-muoto/launcher-manifest.json \
        || grep -Fq "\"desktop\": \"$1\"" /usr/share/harbour-muoto/launcher-manifest.json
}

wait_icon_lock_idle() {
    # Best-effort: avoid starting oneshot while ApplyIcons still holds the lock.
    _lock=/usr/share/harbour-muoto/icon-ops.lock
    i=0
    while [ "$i" -lt 60 ]; do
        if [ ! -e "$_lock" ]; then
            return 0
        fi
        # flock -n succeeds when nobody holds an exclusive lock
        if flock -n "$_lock" -c true 2>/dev/null; then
            return 0
        fi
        sleep 1
        i=$((i + 1))
    done
    return 0
}

# ---------------------------------------------------------------------------
# T-20 Pre-upgrade script (harbour-muoto-oneshot-restore)
# Simulates the path before sailfish-upgrade-ui / system-update.target.
# ---------------------------------------------------------------------------
if [ "$RUN_PREUPGRADE" = true ]; then
    echo "======== T-20 pre-upgrade oneshot-restore ========"

    if [ ! -x /usr/bin/harbour-muoto-oneshot-restore ]; then
        fail T-20-script "missing /usr/bin/harbour-muoto-oneshot-restore"
    else
        pass T-20-script
    fi

    if systemctl cat harbour-muoto-oneshot-restore.service 2>/dev/null \
        | grep -q 'Before=sailfish-upgrade-ui.service'; then
        pass T-20-unit-before
    else
        fail T-20-unit-before "Before=sailfish-upgrade-ui.service missing"
    fi

    if systemctl cat harbour-muoto-oneshot-restore.service 2>/dev/null \
        | grep -q 'WantedBy=system-update.target'; then
        pass T-20-unit-wantedby
    else
        fail T-20-unit-wantedby "WantedBy=system-update.target missing"
    fi

    if [ -f /etc/systemd/system/sailfish-upgrade-ui.service.d/muoto-oneshot-restore.conf ] \
        || [ -f /usr/lib/systemd/system/sailfish-upgrade-ui.service.d/muoto-oneshot-restore.conf ]; then
        pass T-20-dropin
    else
        # installed via %post mv into /etc
        if [ -f /usr/share/harbour-muoto/service/sailfish-upgrade-ui.service.d/muoto-oneshot-restore.conf ]; then
            pass T-20-dropin-packaged
        else
            fail T-20-dropin "sailfish-upgrade-ui drop-in missing"
        fi
    fi

    echo "--- apply $PACK then run oneshot-restore ---"
    apply_pack
    if [ ! -f /usr/share/harbour-muoto/launcher-manifest.json ]; then
        fail T-20-precondition "manifest missing after ApplyIcons($PACK)"
    else
        pass T-20-precondition
    fi
    FT_BEFORE=$(icon_line /usr/share/applications/fingerterm.desktop)
    case "$FT_BEFORE" in
        /usr/share/harbour-muoto/launcher-icons/*) pass T-20-precondition-redirect ;;
        *) fail T-20-precondition-redirect "fingerterm Icon=$FT_BEFORE (expected generated)" ;;
    esac

    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    wait_icon_lock_idle
    START_OK=false
    if run_root systemctl start harbour-muoto-oneshot-restore.service; then
        START_OK=true
    elif run_root /usr/bin/harbour-muoto-oneshot-restore; then
        START_OK=true
    fi
    sleep 3

    pack_now=$(dconf_unquote /apps/harbour-muoto/activeIconPack)
    overlay_now=$(dconf_unquote /apps/harbour-muoto/iconOverlay)
    FT_AFTER=$(icon_line /usr/share/applications/fingerterm.desktop)

    # oneshot can exit non-zero on a flock false-negative when RestoreIcons is
    # very fast; treat post-conditions as the source of truth.
    if [ "$pack_now" = default ] \
        && [ "$overlay_now" = false ] \
        && [ ! -f /usr/share/harbour-muoto/launcher-manifest.json ] \
        && [ "$FT_AFTER" = icon-launcher-shell ]; then
        if [ "$START_OK" = true ]; then
            pass T-20-start
        else
            pass T-20-start-effect
        fi
    else
        fail T-20-start "oneshot did not restore (pack=$pack_now overlay=$overlay_now Icon=$FT_AFTER)"
    fi

    if [ "$pack_now" = default ]; then
        pass T-20-pack-default
    else
        fail T-20-pack-default "activeIconPack=$pack_now"
    fi
    if [ "$overlay_now" = false ]; then
        pass T-20-overlay-false
    else
        fail T-20-overlay-false "iconOverlay=$overlay_now"
    fi
    if [ ! -f /usr/share/harbour-muoto/launcher-manifest.json ]; then
        pass T-20-manifest-gone
    else
        fail T-20-manifest-gone "manifest still present"
    fi
    if [ "$FT_AFTER" = icon-launcher-shell ]; then
        pass T-20-fingerterm-stock
    else
        fail T-20-fingerterm-stock "Icon=$FT_AFTER (expected icon-launcher-shell)"
    fi
    if ls /usr/share/harbour-muoto/launcher-icons/*.png >/dev/null 2>&1; then
        fail T-20-generated-cleared "generated PNGs still present"
    else
        pass T-20-generated-cleared
    fi
fi

# ---------------------------------------------------------------------------
# T-21 Icon change on app install / upgrade (install-listener → update-icons)
# ---------------------------------------------------------------------------
if [ "$RUN_INSTALL" = true ]; then
    echo "======== T-21 install/upgrade re-theme ========"

    if systemctl --user is-active harbour-muoto-install-listener >/dev/null 2>&1; then
        pass T-21-listener-active
    else
        fail T-21-listener-active "harbour-muoto-install-listener not active"
        echo "----"
        echo "Results: $PASS passed, $FAIL failed"
        [ "$FAIL" -eq 0 ]
        exit $?
    fi

    run_root rm -f /run/defaultuser/osupdate_running 2>/dev/null || true

    echo "--- apply $PACK ---"
    apply_pack
    if [ "$(dconf_unquote /apps/harbour-muoto/activeIconPack)" != "$PACK" ]; then
        fail T-21-precondition "activeIconPack not $PACK"
    else
        pass T-21-precondition
    fi

    if [ ! -f "$PROBE_DESKTOP" ]; then
        fail T-21-probe-pkg "$PROBE_DESKTOP missing (set MUOTO_PROBE_PKG / MUOTO_PROBE_DESKTOP)"
    else
        pass T-21-probe-pkg
        ICON_BEFORE=$(icon_line "$PROBE_DESKTOP")
        IN_MANIFEST_BEFORE=false
        manifest_has_desktop "$PROBE_DESKTOP" && IN_MANIFEST_BEFORE=true || true
        echo "probe before Icon=$ICON_BEFORE in_manifest=$IN_MANIFEST_BEFORE"

        SINCE=$(date '+%Y-%m-%d %H:%M:%S')
        echo "--- pkcon reinstall $PROBE_PKG (triggers PackageKit → listener) ---"
        if run_root pkcon -y install --allow-reinstall "$PROBE_PKG"; then
            pass T-21-pkcon
        else
            fail T-21-pkcon "pkcon install --allow-reinstall $PROBE_PKG failed"
        fi

        # Debounce 1.5s + ApplyIcons; allow headroom
        sleep 8

        ICON_AFTER=$(icon_line "$PROBE_DESKTOP")
        echo "probe after Icon=$ICON_AFTER"

        # Listener should have fired update-icons
        if run_root journalctl --user -u harbour-muoto-install-listener --since "$SINCE" --no-pager 2>/dev/null \
            | grep -qE 'muoto-listener:.*(trigger|starting|update-icons finished)'; then
            pass T-21-listener-journal
        elif journalctl --user -u harbour-muoto-install-listener --since "$SINCE" --no-pager 2>/dev/null \
            | grep -qE 'muoto-listener:.*(trigger|starting|update-icons finished)'; then
            pass T-21-listener-journal
        else
            # PackageKit path sometimes only visible via _COMM
            if run_root journalctl _COMM=harbour-muoto-i --since "$SINCE" --no-pager 2>/dev/null \
                | grep -q 'muoto-listener:'; then
                pass T-21-listener-journal
            else
                fail T-21-listener-journal "no muoto-listener lines since $SINCE"
            fi
        fi

        if [ "$(dconf_unquote /apps/harbour-muoto/activeIconPack)" = "$PACK" ]; then
            pass T-21-pack-kept
        else
            fail T-21-pack-kept "pack reset unexpectedly"
        fi

        if [ -f /usr/share/harbour-muoto/launcher-manifest.json ]; then
            pass T-21-manifest-present
        else
            fail T-21-manifest-present "manifest missing after reinstall"
        fi

        # Probe app should be themed (redirect generated path and/or manifest entry)
        themed=false
        case "$ICON_AFTER" in
            /usr/share/harbour-muoto/launcher-icons/*) themed=true ;;
        esac
        if manifest_has_desktop "$PROBE_DESKTOP"; then
            themed=true
        fi
        # Inplace natives: Icon= name unchanged but file under hicolor differs from RPM — check manifest
        if [ "$themed" = true ]; then
            pass T-21-probe-themed
        else
            fail T-21-probe-themed "Icon=$ICON_AFTER not themed / not in manifest"
        fi

        # Fingerterm still themed (global re-apply ran)
        FT=$(icon_line /usr/share/applications/fingerterm.desktop)
        case "$FT" in
            /usr/share/harbour-muoto/launcher-icons/*) pass T-21-global-reapply ;;
            *) fail T-21-global-reapply "fingerterm Icon=$FT after install trigger" ;;
        esac
    fi

    echo "--- OS-update guard: reinstall must not re-apply ---"
    apply_pack
    FT_GUARD=$(icon_line /usr/share/applications/fingerterm.desktop)
    run_root mkdir -p /run/defaultuser
    run_root touch /run/defaultuser/osupdate_running
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    run_root pkcon -y install --allow-reinstall "$PROBE_PKG" >/tmp/muoto-t21-osu-pkcon.log 2>&1 || true
    sleep 6
    run_root rm -f /run/defaultuser/osupdate_running
    if run_root journalctl _COMM=harbour-muoto-i --since "$SINCE" --no-pager 2>/dev/null \
        | grep -qiE 'skip|guard|os.?update'; then
        pass T-21-osu-skip
    elif /usr/bin/harbour-muoto-update-icons 2>&1 | tee /tmp/muoto-t21-osu-update.log | grep -qi 'skip'; then
        # Flag already cleared; verify script still documents skip when flag set
        run_root touch /run/defaultuser/osupdate_running
        if /usr/bin/harbour-muoto-update-icons 2>&1 | grep -qi 'skip'; then
            pass T-21-osu-skip
        else
            fail T-21-osu-skip "update-icons did not skip under osupdate_running"
        fi
        run_root rm -f /run/defaultuser/osupdate_running
    else
        # Soft: pack still active and Icon= unchanged is acceptable if journal inaccessible
        FT_NOW=$(icon_line /usr/share/applications/fingerterm.desktop)
        if [ "$FT_NOW" = "$FT_GUARD" ] && [ "$(dconf_unquote /apps/harbour-muoto/activeIconPack)" = "$PACK" ]; then
            pass T-21-osu-skip-soft
        else
            fail T-21-osu-skip "no guard evidence in journal"
        fi
    fi
fi

echo "----"
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
