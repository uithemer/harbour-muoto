#ifndef LAUNCHERWATCH_H
#define LAUNCHERWATCH_H

#include "muotolauncherglobal.h"
#include <QObject>
#include <QStringList>

// Restore Lipstick's per-file inotify watch on desktop entries whose inode was
// replaced behind its back. apkd rewrites apkd_launcher_*.desktop with
// rename(2); that drops the watch for good, because Lipstick's LauncherMonitor
// only re-watches names it has not seen in the directory before. Until the
// watch is back, an Icon= rewrite is invisible and the tile needs a homescreen
// restart.
//
// The waits used to be nested QEventLoops, which is what made the daemon
// re-entrant: a D-Bus call, the desktop-directory watcher or a 60 s dynamic tick
// could all run *inside* a re-arm, while entries were renamed aside. That is the
// window in which a write to an entry that was not there produced a stub. This
// is the same sequence driven by single-shot timers instead, so the daemon keeps
// answering D-Bus without anything else starting icon work underneath it.
class MUOTO_LAUNCHER_EXPORT LauncherRearm : public QObject
{
    Q_OBJECT

public:
    explicit LauncherRearm(QObject* parent = nullptr);
    ~LauncherRearm() override;

    // Renames each entry aside and back with a gap, then sits out the holdback.
    // Emits finished() when the entries are back and the watches are live.
    void start(const QStringList& desktopPaths);

    // Just the holdback, for callers that only need to let the grid settle.
    void startHoldbackOnly();

    // Entries currently renamed aside, so a shutdown can put them back. Losing
    // these across an uninstall means the user loses those launcher items for
    // good: nothing is left to sweep them.
    QStringList asidePaths() const { return m_moved; }

    // Synchronous, for SIGTERM only.
    void abortAndRestore();

signals:
    void finished();
    // Coarse liveness while the state machine runs. The GUI watchdog treats any
    // progress as proof the daemon is alive rather than wedged, and re-arm plus
    // holdback is ~5 s of otherwise total silence.
    void heartbeat();

private:
    void moveBack();
    void done();

    QStringList m_moved;
    bool m_running = false;
};

namespace LauncherWatch {

// Put back entries left renamed aside by a re-arm that did not finish.
MUOTO_LAUNCHER_EXPORT void sweepStaleRearmFiles(const QStringList& directories);

} // namespace LauncherWatch

#endif // LAUNCHERWATCH_H
