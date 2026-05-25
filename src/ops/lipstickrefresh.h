#ifndef LIPSTICKREFRESH_H
#define LIPSTICKREFRESH_H

#include <QString>

bool homeRefreshEnabledInDconf();

bool touchPathForLauncher(const QString& path);

bool restartDefaultUserLipstick();

void touchAllLauncherDesktops();

bool applyApkPhase(const QString& packName, bool runPack, bool overlay,
                   bool* apkIconsTouched = nullptr);

void revertApkDesktopsToLauncherIcon();

void removeApkCustomDir();

void notifyLauncherAfterIconOp(bool apkIconsTouched = false);

#endif // LIPSTICKREFRESH_H
