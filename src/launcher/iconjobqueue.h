#ifndef ICONJOBQUEUE_H
#define ICONJOBQUEUE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <memory>

class FileLock;
class LauncherIconOps;

// Serialises launcher icon work so Apply / restore / APK refresh / new-desktop
// scans / dyn ticks never nest. One job runs at a time; FileLock is held for
// that job including async re-arm waits (not across the whole drain).
class IconJobQueue : public QObject
{
    Q_OBJECT

public:
    enum class Kind
    {
        ApplyAll,
        Restore,
        RefreshDesktops,
        RefreshApk,
        RebuildDyn,
        Rebuild
    };

    explicit IconJobQueue(LauncherIconOps* ops, QObject* parent = nullptr);
    ~IconJobQueue() override;

    void enqueueApply(const QString& pack, bool runPack, bool overlay);
    void enqueueRestore();
    void enqueueRefreshDesktops();
    void enqueueRefreshApk(bool scheduleVerify);
    void enqueueRebuildDyn(const QString& desktopPath);
    void enqueueRebuild();

    bool isRunning() const { return m_running != nullptr; }

    // Apply / Restore / Rebuild end in rebuildIconUpdatersNow — dconf watches
    // that fire because of those writes must not enqueue a second rebuild.
    bool runningJobEndsInFullRebuild() const;

    // Called by LauncherIconOps when the async job body has finished.
    void jobFinished();

private:
    struct Job
    {
        Kind kind = Kind::Rebuild;
        QString pack;
        bool runPack = true;
        bool overlay = false;
        bool scheduleVerify = false;
        QStringList dynPaths;
    };

    void kick();
    void startJob(Job job);
    void coalesce(Job&& job);

    LauncherIconOps* m_ops = nullptr;
    QVector<Job> m_pending;
    std::unique_ptr<Job> m_running;
    std::unique_ptr<FileLock> m_lock;
};

#endif // ICONJOBQUEUE_H
