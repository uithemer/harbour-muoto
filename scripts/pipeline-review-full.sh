#!/bin/sh
# Full on-device pipeline review after redeploy (defaultuser).
set -eu

SNAP() {
    echo "pack=$(dconf read /apps/harbour-muoto/activeIconPack) overlay=$(dconf read /apps/harbour-muoto/iconOverlay) icons=$(ls /usr/share/harbour-muoto/launcher-icons/*.png 2>/dev/null | wc -l) manifest=$(test -f /usr/share/harbour-muoto/launcher-manifest.json && wc -c </usr/share/harbour-muoto/launcher-manifest.json || echo 0)"
}

JLOG() {
    echo rootme | sudo -S journalctl _COMM=harbour-muoto-l --since "$1" --no-pager 2>/dev/null \
        | grep -E "muoto-launcher:" | tail -"${2:-30}" || true
}

PIPE="${1:-all}"
SINCE=$(date '+%Y-%m-%d %H:%M:%S')

case "$PIPE" in
p1|all)
    echo "======== P1 baseline ========"
    test -f /usr/lib/systemd/user/harbour-muoto-launcher-icond.service && echo "PASS unit in /usr/lib/systemd/user"
    systemctl --user is-active harbour-muoto-launcher-icond
    echo rootme | sudo -S getcap /usr/libexec/harbour-muoto-launcher-icond
    dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus \
        org.freedesktop.DBus.GetNameOwner string:org.muoto.Launcher1 | tail -1

    echo "--- P1 edge: stop + ensure via update-icons (pack default = no-op); use ensure script ---"
    systemctl --user stop harbour-muoto-launcher-icond
    sleep 1
    . /usr/share/harbour-muoto/service/muoto-dbus-wait.sh
    muoto_ensure_launcher_icond
    systemctl --user is-active harbour-muoto-launcher-icond
    dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus \
        org.freedesktop.DBus.GetNameOwner string:org.muoto.Launcher1 | tail -1

    echo "--- P1 edge: kill Restart=always ---"
    PID=$(pgrep -n harbour-muoto-launcher-icond)
    kill "$PID"
    sleep 3
    systemctl --user is-active harbour-muoto-launcher-icond
    systemctl --user enable --now harbour-muoto-launcher-icond 2>/dev/null || true
    ;;
esac

case "$PIPE" in
p2|all)
    echo "======== P2 ApplyIcons ========"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    SNAP
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 4
    SNAP
    grep -H "^Icon=" /usr/share/applications/fingerterm.desktop /usr/share/applications/jolla-clock.desktop || true
    echo "--- journal ---"
    JLOG "$SINCE" 20

    echo "--- unknown pack ---"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    dbus-monitor --session "type='signal',interface='org.muoto.Launcher1.Themes'" >/tmp/p2-unk.mon 2>&1 &
    MON=$!; sleep 0.3
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:no-such-pack boolean:true boolean:false
    sleep 1; kill $MON 2>/dev/null || true
    grep -A5 OperationCompleted /tmp/p2-unk.mon | head -10
    JLOG "$SINCE" 10

    echo "--- busy ---"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    (
        flock -x 9
        sleep 5
    ) 9>/usr/share/harbour-muoto/icon-ops.lock &
    sleep 1
    dbus-monitor --session "type='signal',interface='org.muoto.Launcher1.Themes'" >/tmp/p2-busy.mon 2>&1 &
    MON=$!; sleep 0.3
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 2; kill $MON 2>/dev/null || true; wait 2>/dev/null || true
    grep -A5 OperationCompleted /tmp/p2-busy.mon | head -10
    JLOG "$SINCE" 10

    echo "--- os-update ---"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    echo rootme | sudo -S mkdir -p /run/defaultuser
    echo rootme | sudo -S touch /run/defaultuser/osupdate_running
    dbus-monitor --session "type='signal',interface='org.muoto.Launcher1.Themes'" >/tmp/p2-osu.mon 2>&1 &
    MON=$!; sleep 0.3
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 1; kill $MON 2>/dev/null || true
    echo rootme | sudo -S rm -f /run/defaultuser/osupdate_running
    grep -A5 OperationCompleted /tmp/p2-osu.mon | head -10
    JLOG "$SINCE" 10
    ;;
esac

case "$PIPE" in
p3|all)
    echo "======== P3 RestoreIcons ========"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    # ensure themed
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    SNAP
    dbus-monitor --session "type='signal',interface='org.muoto.Launcher1.Themes'" >/tmp/p3.mon 2>&1 &
    MON=$!; sleep 0.3
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.RestoreIcons
    sleep 3; kill $MON 2>/dev/null || true
    grep -A5 OperationCompleted /tmp/p3.mon | head -10
    SNAP
    grep -H "^Icon=" /usr/share/applications/fingerterm.desktop /usr/share/applications/jolla-clock.desktop || true
    JLOG "$SINCE" 25

    echo "--- idempotent ---"
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.RestoreIcons
    sleep 1
    ;;
esac

case "$PIPE" in
p4|all)
    echo "======== P4 overlay-only ========"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.RestoreIcons
    sleep 2
    echo "--- cairo runPack=false (no overlay) ---"
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:cairo boolean:false boolean:true
    sleep 3
    SNAP
    grep -H "^Icon=" /usr/share/applications/fingerterm.desktop || true
    JLOG "$SINCE" 15

    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    echo "--- haiku runPack=false overlay=true ---"
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:false boolean:true
    sleep 4
    SNAP
    python3 -c "import json; d=json.load(open('/usr/share/harbour-muoto/launcher-manifest.json')); print('count',len(d),'modes',set(e['mode'] for e in d))" 2>/dev/null || true
    JLOG "$SINCE" 15
    ;;
esac

case "$PIPE" in
p5|all)
    echo "======== P5 cover-equiv ========"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    PACK=$(dconf read /apps/harbour-muoto/activeIconPack); PACK=${PACK#\'}; PACK=${PACK%\'}
    OVER=$(dconf read /apps/harbour-muoto/iconOverlay)
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:"$PACK" boolean:true boolean:"$OVER"
    sleep 3
    SNAP
    JLOG "$SINCE" 15
    ;;
esac

case "$PIPE" in
p6|all)
    echo "======== P6 update-icons ========"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    /usr/bin/harbour-muoto-update-icons 2>&1 | tee /tmp/p6.log | tail -15
    sleep 2
    SNAP
    JLOG "$SINCE" 15

    echo "--- OS update skip ---"
    echo rootme | sudo -S touch /run/defaultuser/osupdate_running
    /usr/bin/harbour-muoto-update-icons 2>&1 | tee /tmp/p6-osu.log | tail -10
    echo rootme | sudo -S rm -f /run/defaultuser/osupdate_running

    echo "--- default skip ---"
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.RestoreIcons
    sleep 2
    /usr/bin/harbour-muoto-update-icons 2>&1 | tee /tmp/p6-def.log | tail -10
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    ;;
esac

case "$PIPE" in
p7|all)
    echo "======== P7 install-listener ========"
    systemctl --user is-active harbour-muoto-install-listener
    echo rootme | sudo -S rm -f /run/defaultuser/osupdate_running
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    echo rootme | sudo -S pkcon -y install --allow-reinstall harbour-file-browser 2>&1 | tail -15
    sleep 5
    echo rootme | sudo -S journalctl _COMM=harbour-muoto-i --since "$SINCE" --no-pager 2>/dev/null \
        | grep -E "muoto-listener:" | tail -25

    echo "--- OS update guard ---"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    echo rootme | sudo -S touch /run/defaultuser/osupdate_running
    echo rootme | sudo -S pkcon -y install --allow-reinstall harbour-file-browser 2>&1 | tail -8
    sleep 4
    echo rootme | sudo -S rm -f /run/defaultuser/osupdate_running
    echo rootme | sudo -S journalctl _COMM=harbour-muoto-i --since "$SINCE" --no-pager 2>/dev/null \
        | grep -E "muoto-listener:" | tail -15
    ;;
esac

case "$PIPE" in
p8|all)
    echo "======== P8 per-app ========"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    KEY=/apps/harbour-muoto/launcher/applications/fingerterm/provider
    REL=$(ls /usr/share/harbour-themepack-haiku/jolla/*/icons/icon-launcher-browser.png | head -1 | sed 's|.*/harbour-themepack-haiku/||')
    BEFORE=$(grep "^Icon=" /usr/share/applications/fingerterm.desktop)
    SIZE_B=$(stat -c%s "$(echo "$BEFORE" | cut -d= -f2-)")
    dconf write "$KEY" "\"icon-pack://haiku/$REL\""
    sleep 4
    AFTER=$(grep "^Icon=" /usr/share/applications/fingerterm.desktop)
    SIZE_A=$(stat -c%s "$(echo "$AFTER" | cut -d= -f2-)")
    echo "same-pack before=$SIZE_B after=$SIZE_A"
    echo "$BEFORE -> $AFTER"
    JLOG "$SINCE" 15

    echo "--- cross-pack cairo (on-demand load) ---"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    CREL=$(ls /usr/share/harbour-themepack-cairo/jolla/*/icons/icon-launcher-browser.png 2>/dev/null | head -1 | sed 's|.*/harbour-themepack-cairo/||' || true)
    if [ -n "$CREL" ]; then
        dconf write "$KEY" "\"icon-pack://cairo/$CREL\""
        sleep 4
        AFTER2=$(grep "^Icon=" /usr/share/applications/fingerterm.desktop)
        SIZE_C=$(stat -c%s "$(echo "$AFTER2" | cut -d= -f2-)" 2>/dev/null || echo 0)
        echo "cross-pack $AFTER2 size=$SIZE_C"
        JLOG "$SINCE" 15
    else
        echo "SKIP no cairo browser icon"
    fi

    echo "--- bare icon name ---"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    dconf write "$KEY" "\"icon-pack://haiku/icon-launcher-calculator\""
    sleep 4
    AFTER3=$(grep "^Icon=" /usr/share/applications/fingerterm.desktop)
    SIZE_D=$(stat -c%s "$(echo "$AFTER3" | cut -d= -f2-)" 2>/dev/null || echo 0)
    echo "bare-name $AFTER3 size=$SIZE_D"
    JLOG "$SINCE" 15

    dconf reset "$KEY" 2>/dev/null || true
    sleep 3
    ;;
esac

case "$PIPE" in
p9|all)
    echo "======== P9 lifecycle ========"
    SINCE=$(date '+%Y-%m-%d %H:%M:%S')
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    SNAP
    echo rootme | sudo -S systemctl start harbour-muoto-oneshot-restore.service
    sleep 6
    echo rootme | sudo -S journalctl -u harbour-muoto-oneshot-restore --since "$SINCE" --no-pager 2>/dev/null | grep -E "muoto:|Restore" | tail -20
    SNAP
    JLOG "$SINCE" 20
    grep -H "^Icon=" /usr/share/applications/fingerterm.desktop || true

    echo "--- --restore-once ---"
    dbus-send --session --type=method_call --dest=org.muoto.Launcher1 /org/muoto/Launcher1 \
        org.muoto.Launcher1.Themes.ApplyIcons string:haiku boolean:true boolean:true
    sleep 3
    SNAP
    systemctl --user stop harbour-muoto-launcher-icond
    sleep 1
    /usr/libexec/harbour-muoto-launcher-icond --restore-once; echo rc=$?
    SNAP
    grep -H "^Icon=" /usr/share/applications/fingerterm.desktop || true
    # leave clean: reset dconf then start icond
    dconf write /apps/harbour-muoto/activeIconPack "'default'"
    dconf write /apps/harbour-muoto/iconOverlay false
    systemctl --user start harbour-muoto-launcher-icond
    sleep 2
    SNAP
    systemctl --user is-active harbour-muoto-launcher-icond harbour-muoto-install-listener
    ;;
esac

case "$PIPE" in
p10|all)
    echo "======== P10 pre-upgrade oneshot-restore (see device-test-preupgrade-install.sh T-20) ========"
    if [ -f "$(dirname "$0")/device-test-preupgrade-install.sh" ]; then
        sh "$(dirname "$0")/device-test-preupgrade-install.sh" --pack haiku --skip-install --skip-folder --skip-dyn
    else
        echo "SKIP: copy scripts/device-test-preupgrade-install.sh to device"
    fi
    ;;
esac

case "$PIPE" in
p11|all)
    echo "======== P11 install/upgrade re-theme (see device-test-preupgrade-install.sh T-21) ========"
    if [ -f "$(dirname "$0")/device-test-preupgrade-install.sh" ]; then
        sh "$(dirname "$0")/device-test-preupgrade-install.sh" --pack haiku --skip-preupgrade --skip-folder --skip-dyn
    else
        echo "SKIP: copy scripts/device-test-preupgrade-install.sh to device"
    fi
    ;;
esac

case "$PIPE" in
p12|all)
    echo "======== P12 silica folder ambient (T-22) ========"
    if [ -f "$(dirname "$0")/device-test-preupgrade-install.sh" ]; then
        sh "$(dirname "$0")/device-test-preupgrade-install.sh" --pack haiku --skip-preupgrade --skip-install --skip-dyn
    else
        echo "SKIP: copy scripts/device-test-preupgrade-install.sh to device"
    fi
    ;;
esac

case "$PIPE" in
p13|all)
    echo "======== P13 dynamic icons (T-23) ========"
    if [ -f "$(dirname "$0")/device-test-preupgrade-install.sh" ]; then
        sh "$(dirname "$0")/device-test-preupgrade-install.sh" --pack haiku --skip-preupgrade --skip-install --skip-folder
    else
        echo "SKIP: copy scripts/device-test-preupgrade-install.sh to device"
    fi
    ;;
esac

echo "======== DONE $PIPE ========"
