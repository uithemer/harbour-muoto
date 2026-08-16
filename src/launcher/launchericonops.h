#ifndef LAUNCHERICONOPS_H
#define LAUNCHERICONOPS_H

#include "muotolauncherglobal.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <functional>

class QFileSystemWatcher;
class IconJobQueue;

class MUOTO_LAUNCHER_EXPORT LauncherIconOps : public QObject
{
    Q_OBJECT

public:
    static LauncherIconOps* instance();

    void applyIcons(const QString& pack, bool runPack, bool overlay);
    void restoreIcons();

    bool applyPackIcons() const { return m_applyPackIcons; }

    bool restoreOnUpdaterDestroy() const { return m_restoreOnUpdaterDestroy; }

    void rebuildIconUpdaters();

    // True while the job queue is running a job (including async re-arm).
    bool isJobRunning() const;

    // Dyn tick: coalesce per-desktop updates through the queue.
    void enqueueRebuildDyn(const QString& desktopPath);

    // Recover aside files and stop the re-arm state machine before quit.
    void prepareShutdown();

    // Whether an IconUpdater is already attached (brand-new installs have none).
    bool hasUpdater(const QString& desktopPath) const;

signals:
    void progress(int done, int total);
    void applied(bool ok, const QString& message);
    void restored(bool ok, const QString& message);

private:
    friend class IconJobQueue;

    explicit LauncherIconOps(QObject* parent = nullptr);

    void emitApplyFinished(bool ok, const QString& message);
    void emitRestoreFinished(bool ok, const QString& message);

    void runApplyIcons(const QString& pack, bool runPack, bool overlay);
    void runRestoreIcons();
    void runRefreshNewDesktops();
    void runRefreshApkIcons(bool scheduleVerify);
    void runRebuildDyn(const QStringList& desktopPaths);
    void runRebuild();

    void rebuildIconUpdatersNow();
    void clearUpdaters(bool restoreOnDestroy);
    void reloadIconPacks();
    void ensureDesktopWatches();
    void ensureDesktopDirWatch();

    QStringList desktopsNeedingTheme() const;
    QStringList desktopsNeedingWatchRearm(const QStringList& candidates) const;
    void rearmThen(const QStringList& candidates, const std::function<void()>& next);

    bool apkIconsClobbered() const;

    void emitProgress(int done, int total);
    void finishJob();

    IconJobQueue* m_queue = nullptr;
    QFileSystemWatcher* m_desktopDirWatcher = nullptr;
    QTimer m_desktopScan;

    bool m_restoreOnUpdaterDestroy = true;
    bool m_applyPackIcons = true;
    bool m_rebuilding = false;
    int m_progressTotal = 0;
    int m_progressPercent = -1;
};

#endif // LAUNCHERICONOPS_H
