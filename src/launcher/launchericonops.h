#ifndef LAUNCHERICONOPS_H
#define LAUNCHERICONOPS_H

#include "iconjob.h"
#include "launcherwatch.h"
#include "muotolauncherglobal.h"
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QTimer>

class QFileSystemWatcher;

class MUOTO_LAUNCHER_EXPORT LauncherIconOps : public QObject
{
    Q_OBJECT

public:
    static LauncherIconOps* instance();

    // Runs one job to completion and then emits jobFinished(). Only IconJobQueue
    // calls this: it is what guarantees a single operation at a time, which the
    // old design could not do because the Lipstick waits were nested event loops
    // that let the next trigger start work underneath the current one.
    void startJob(const IconJob& job);

    bool applyPackIcons() const { return m_applyPackIcons; }

    // Put back anything a re-arm currently has renamed aside, synchronously.
    // On uninstall there is no second chance: %preun stops the daemon and then
    // removes it, so an entry left aside is a launcher item the user loses for
    // good with nothing left to sweep it.
    void restoreAsideEntries();

signals:
    void progress(int done, int total);
    void jobFinished(bool ok, const QString& message);

private:
    explicit LauncherIconOps(QObject* parent = nullptr);

    void runApply();
    void runRestore();
    void runRefreshDesktops();
    void runRefreshApk();
    void runRebuild();

    // Phase 2 of a full operation: everything between the re-arm finishing and
    // the Lipstick holdback starting.
    void applyAfterRearm();
    void restoreAfterRearm();
    void refreshDesktopsAfterRearm();
    void refreshApkAfterRearm();

    void startHoldbackThen(const QString& message, bool ok);
    void finishJob(bool ok, const QString& message);
    void rearmThen(const QStringList& paths, void (LauncherIconOps::*next)());

    void rebuildIconUpdatersNow();
    // Drops generated PNGs nothing references; keeps the ones a desktop or a
    // manifest entry still names.
    void reconcileGeneratedIcons();
    // restoreFirst: put the stock icons back before dropping the updaters. False
    // when they are about to be recreated for the same entries.
    void clearUpdaters(bool restoreFirst);
    void reloadIconPacks();
    void ensureDesktopWatches();

    bool apkIconsClobbered() const;

    // Theme apps installed or updated behind our back, whatever installer did it.
    void ensureDesktopDirWatch();
    QStringList desktopsNeedingTheme() const;

    // Silent unless an apply/restore is in flight: rebuildIconUpdatersNow()
    // also runs from dconf watches, where there is nothing to report.
    void emitProgress(int done, int total);

    QFileSystemWatcher* m_desktopDirWatcher = nullptr;
    QTimer m_desktopScan;

    IconJob m_job;
    QScopedPointer<LauncherRearm> m_rearm;

    // Reset per rebuild. Apply reports ok / partial / hard failure from these
    // instead of assuming success, so a device that themed nothing (a lost
    // cap_dac_override, say) stops toasting "Theme updated."
    int m_updatersBuilt = 0;
    int m_updatersWritten = 0;

    bool m_applyPackIcons = true;
    // True for the duration of a job, purely so progress is reported for real
    // work and not for a dconf-driven rebuild nobody asked about.
    bool m_inIconOp = false;
    bool m_rebuilding = false;
    // Desktop entries plus one step for the folder-tile pass that follows.
    int m_progressTotal = 0;
    // Last reported percentage, used to rate-limit the progress signal.
    int m_progressPercent = -1;
};

#endif // LAUNCHERICONOPS_H
