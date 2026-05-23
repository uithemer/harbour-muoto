#ifndef LIPSTICKREFRESH_H
#define LIPSTICKREFRESH_H

#include <QString>

// Whether defaultuser asked for lipstick.service restart after icon ops.
bool homeRefreshEnabledInDconf();

// Bump mtime so Lipstick's QFileSystemWatcher notices the path (Clockwork pattern).
bool touchPathForLauncher(const QString& path);

// Restart defaultuser's lipstick.service so launcher icons reload after
// .desktop Icon= changes. Must run from a context that can reach the user
// systemd session (helperd as root with XDG_RUNTIME_DIR set).
bool restartDefaultUserLipstick();

// Touch native and apkd launcher .desktop files (TPS reapply pattern).
void touchAllLauncherDesktops();

// Touch desktops then optional Lipstick restart when homeRefresh dconf is true.
void notifyLauncherAfterIconOp();

#endif // LIPSTICKREFRESH_H
