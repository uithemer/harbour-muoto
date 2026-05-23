#ifndef LIPSTICKREFRESH_H
#define LIPSTICKREFRESH_H

#include <QString>

// Whether defaultuser asked for lipstick.service restart (dconf homeRefresh).
bool homeRefreshEnabledInDconf();

// Bump mtime so Lipstick's QFileSystemWatcher notices the path (Clockwork pattern).
bool touchPathForLauncher(const QString& path);

// Restart defaultuser's lipstick.service (GUI-only after icon dconf commit).
bool restartDefaultUserLipstick();

// Touch native and apkd launcher .desktop files (TPS reapply pattern).
void touchAllLauncherDesktops();

// Touch launcher desktops after icon apply/restore (no lipstick restart here).
void notifyLauncherAfterIconOp();

#endif // LIPSTICKREFRESH_H
