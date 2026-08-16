#ifndef LAUNCHERWATCH_H
#define LAUNCHERWATCH_H

#include "muotolauncherglobal.h"
#include <QStringList>

#include <functional>

namespace LauncherWatch {

// Restore Lipstick's per-file inotify watch on desktop entries whose inode was
// replaced behind its back. apkd (and native RPM updates) rewrite .desktop files
// with rename(2); that drops the watch for good, because Lipstick's
// LauncherMonitor only re-watches names it has not seen in the directory before.
//
// Aside/back is asynchronous: callers must not nest a QEventLoop. onDone runs
// after the holdback so Icon= writes land on a live watch.
MUOTO_LAUNCHER_EXPORT void rearmDesktopWatches(const QStringList& desktopPaths,
                                               std::function<void()> onDone);

// Sit out Lipstick's LauncherMonitor holdback after desktop/PNG writes.
MUOTO_LAUNCHER_EXPORT void waitForMonitorHoldback(std::function<void()> onDone);

// Put back entries left renamed aside by a re-arm that did not finish.
MUOTO_LAUNCHER_EXPORT void sweepStaleRearmFiles(const QStringList& directories);

// True while an aside/back round-trip is in flight.
MUOTO_LAUNCHER_EXPORT bool rearmInProgress();

// Abort a pending timer continuation and restore any *.muoto-rearm files now.
// Used on SIGTERM / PrepareForShutdown.
MUOTO_LAUNCHER_EXPORT void abortAndRecover(const QStringList& directories);

} // namespace LauncherWatch

#endif // LAUNCHERWATCH_H
