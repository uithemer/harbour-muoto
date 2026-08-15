#ifndef LAUNCHERWATCH_H
#define LAUNCHERWATCH_H

#include "muotolauncherglobal.h"
#include <QStringList>

namespace LauncherWatch {

// Restore Lipstick's per-file inotify watch on desktop entries whose inode was
// replaced behind its back. apkd rewrites apkd_launcher_*.desktop with
// rename(2); that drops the watch for good, because Lipstick's LauncherMonitor
// only re-watches names it has not seen in the directory before. Until the
// watch is back, an Icon= rewrite is invisible and the tile needs a homescreen
// restart.
MUOTO_LAUNCHER_EXPORT void rearmDesktopWatches(const QStringList& desktopPaths);

// Sit out Lipstick's LauncherMonitor holdback after desktop/PNG writes so a
// caller can toast or release the icon-ops lock only once the grid has had a
// chance to reload.
MUOTO_LAUNCHER_EXPORT void waitForMonitorHoldback();

// Put back entries left renamed aside by a re-arm that did not finish.
MUOTO_LAUNCHER_EXPORT void sweepStaleRearmFiles(const QStringList& directories);

} // namespace LauncherWatch

#endif // LAUNCHERWATCH_H
