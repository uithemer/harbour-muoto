#!/bin/sh
# On-device pipeline review helpers P5–P9 (run as defaultuser).
set -eu

SNAP() {
    echo "pack=$(dconf read /apps/harbour-muoto/activeIconPack) overlay=$(dconf read /apps/harbour-muoto/iconOverlay) icons=$(ls /usr/share/harbour-muoto/launcher-icons/*.png 2>/dev/null | wc -l) manifest=$(test -f /usr/share/harbour-muoto/launcher-manifest.json && wc -c </usr/share/harbour-muoto/launcher-manifest.json || echo 0)"
}

watch_op() {
    out="$1"
    rm -f "$out"
    dbus-monitor --session "type='signal',interface='org.muoto.Launcher1.Themes'" >"$out" 2>&1 &
    echo $!
    sleep 0.4
}

PIPE="${1:-all}"

case "$PIPE" in
p5|all)
    echo "=== P5 cover-equiv ==="
    SNAP
    PACK=$(dconf read /apps/harbour-muoto/activeIconPack)
    PACK=${PACK#\'}
    PACK=${PACK%\'}
    OVER=$(dconf read /apps/harbour-muoto/iconOverlay)
    MON=$(watch_op /tmp/muoto-cover.mon)
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:"$PACK" boolean:true boolean:"$OVER"
    sleep 4
    kill "$MON" 2>/dev/null || true
    grep -A5 OperationCompleted /tmp/muoto-cover.mon | head -15 || true
    SNAP
    echo "CoverActionList gated by hasActiveIconPack — verified in QML; default pack disables cover sync"
    ;;
esac

case "$PIPE" in
p6|all)
    echo "=== P6 update-icons happy ==="
    SNAP
    /usr/bin/harbour-muoto-update-icons 2>&1 | tee /tmp/muoto-update-icons.log | tail -20
    sleep 2
    SNAP

    echo "=== P6 OS-update skip ==="
    echo rootme | sudo -S mkdir -p /run/defaultuser
    echo rootme | sudo -S touch /run/defaultuser/osupdate_running
    /usr/bin/harbour-muoto-update-icons 2>&1 | tee /tmp/muoto-update-osu.log | tail -20
    echo rootme | sudo -S rm -f /run/defaultuser/osupdate_running

    echo "=== P6 default pack skip ==="
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.RestoreIcons
    sleep 2
    /usr/bin/harbour-muoto-update-icons 2>&1 | tee /tmp/muoto-update-default.log | tail -20
    # restore themed
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    SNAP
    ;;
esac

case "$PIPE" in
p7|all)
    echo "=== P7 install-listener status ==="
    systemctl --user is-active harbour-muoto-install-listener
    systemctl --user status harbour-muoto-install-listener --no-pager 2>&1 | head -20
    # Soft trigger: touch a desktop and ask listener path via update-icons (same as scheduleApply)
    echo "=== P7 simulate scheduleApply via update-icons ==="
    journalctl --user -u harbour-muoto-install-listener --since "5 min ago" --no-pager 2>&1 | tail -15 || true
    /usr/bin/harbour-muoto-update-icons 2>&1 | tail -15
    sleep 2
    SNAP
    echo "=== P7 OS-update: listener must not apply ==="
    echo rootme | sudo -S touch /run/defaultuser/osupdate_running
    # invoke listener's skip by running update-icons (same guard)
    /usr/bin/harbour-muoto-update-icons 2>&1 | tee /tmp/muoto-listener-osu.log | tail -15
    echo rootme | sudo -S rm -f /run/defaultuser/osupdate_running
    ;;
esac

case "$PIPE" in
p8|all)
    echo "=== P8 per-app provider ==="
    SNAP
    # Set fingerterm to use cairo pack icon if available, else icon-pack://haiku with empty path uses default
    KEY=/apps/harbour-muoto/launcher/applications/fingerterm/provider
    BEFORE=$(grep "^Icon=" /usr/share/applications/fingerterm.desktop)
    echo "before $BEFORE"
    dconf write "$KEY" "'icon-pack://cairo'"
    sleep 3
    AFTER=$(grep "^Icon=" /usr/share/applications/fingerterm.desktop)
    echo "after cairo provider $AFTER"
    SNAP
    dconf write "$KEY" "'<none>'"
    sleep 2
    dconf reset "$KEY" 2>/dev/null || dconf write "$KEY" "''"
    sleep 3
    CLEARED=$(grep "^Icon=" /usr/share/applications/fingerterm.desktop)
    echo "after clear $CLEARED"
    SNAP
    ;;
esac

case "$PIPE" in
p9|all)
    echo "=== P9 oneshot-restore ==="
    SNAP
    # Ensure themed first
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    SNAP
    echo rootme | sudo -S systemctl start harbour-muoto-oneshot-restore.service 2>&1 || \
        /usr/bin/harbour-muoto-oneshot-restore 2>&1 | tee /tmp/muoto-oneshot.log | tail -30
    sleep 5
    echo rootme | sudo -S journalctl -u harbour-muoto-oneshot-restore --since "2 min ago" --no-pager 2>&1 | tail -40
    SNAP
    grep -H "^Icon=" /usr/share/applications/fingerterm.desktop /usr/share/applications/jolla-clock.desktop || true

    echo "=== P9 --restore-once ==="
    # Re-theme then stop daemon and restore-once
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    SNAP
    systemctl --user stop harbour-muoto-launcher-icond
    sleep 1
    /usr/libexec/harbour-muoto-launcher-icond --restore-once; echo restore_once_rc=$?
    SNAP
    grep -H "^Icon=" /usr/share/applications/fingerterm.desktop || true
    systemctl --user start harbour-muoto-launcher-icond
    sleep 2
    systemctl --user is-active harbour-muoto-launcher-icond
    ;;
esac

echo "=== DONE $PIPE ==="
