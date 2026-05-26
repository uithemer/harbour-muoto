#ifndef LIPSTICKREFRESH_H
#define LIPSTICKREFRESH_H

#include <QString>

bool touchPathForLauncher(const QString& path);

void touchAllLauncherDesktops();

bool applyApkPhase(const QString& packName, bool runPack, bool overlay,
                   bool* apkIconsTouched = nullptr);

void removeApkCustomDir();

void notifyLauncherAfterIconOp(bool apkIconsTouched = false);

#endif // LIPSTICKREFRESH_H
