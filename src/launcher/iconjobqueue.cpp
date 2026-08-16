#include "iconjobqueue.h"

#include "filelock.h"
#include "launchericonops.h"

#include <QDebug>

IconJobQueue::IconJobQueue(LauncherIconOps* ops, QObject* parent)
    : QObject(parent)
    , m_ops(ops)
{
}

IconJobQueue::~IconJobQueue() = default;

bool IconJobQueue::runningJobEndsInFullRebuild() const
{
    if(!m_running)
        return false;
    switch(m_running->kind)
    {
    case Kind::ApplyAll:
    case Kind::Restore:
    case Kind::Rebuild:
        return true;
    default:
        return false;
    }
}

void IconJobQueue::enqueueApply(const QString& pack, bool runPack, bool overlay)
{
    Job job;
    job.kind = Kind::ApplyAll;
    job.pack = pack;
    job.runPack = runPack;
    job.overlay = overlay;
    coalesce(std::move(job));
    kick();
}

void IconJobQueue::enqueueRestore()
{
    Job job;
    job.kind = Kind::Restore;
    coalesce(std::move(job));
    kick();
}

void IconJobQueue::enqueueRefreshDesktops()
{
    Job job;
    job.kind = Kind::RefreshDesktops;
    coalesce(std::move(job));
    kick();
}

void IconJobQueue::enqueueRefreshApk(bool scheduleVerify)
{
    Job job;
    job.kind = Kind::RefreshApk;
    job.scheduleVerify = scheduleVerify;
    coalesce(std::move(job));
    kick();
}

void IconJobQueue::enqueueRebuildDyn(const QString& desktopPath)
{
    Job job;
    job.kind = Kind::RebuildDyn;
    if(!desktopPath.isEmpty())
        job.dynPaths.append(desktopPath);
    coalesce(std::move(job));
    kick();
}

void IconJobQueue::enqueueRebuild()
{
    Job job;
    job.kind = Kind::Rebuild;
    coalesce(std::move(job));
    kick();
}

void IconJobQueue::coalesce(Job&& job)
{
    // ApplyAll supersedes pending desktop refreshes and plain rebuilds — it
    // rebuilds everything. Dropping RefreshDesktops here also keeps a just-
    // installed app out of s_updaters until Apply runs, so re-arm skips it.
    if(job.kind == Kind::ApplyAll || job.kind == Kind::Restore)
    {
        QVector<Job> kept;
        kept.reserve(m_pending.size());
        for(Job& pending : m_pending)
        {
            if(pending.kind == Kind::RefreshDesktops
               || pending.kind == Kind::Rebuild
               || pending.kind == Kind::RebuildDyn)
                continue;
            if(job.kind == Kind::ApplyAll && pending.kind == Kind::ApplyAll)
                continue; // replaced by latest args below
            if(job.kind == Kind::Restore && pending.kind == Kind::Restore)
                continue;
            kept.append(std::move(pending));
        }
        m_pending = std::move(kept);
        m_pending.append(std::move(job));
        return;
    }

    if(job.kind == Kind::RefreshApk)
    {
        for(Job& pending : m_pending)
        {
            if(pending.kind != Kind::RefreshApk)
                continue;
            pending.scheduleVerify = pending.scheduleVerify || job.scheduleVerify;
            return;
        }
        m_pending.append(std::move(job));
        return;
    }

    if(job.kind == Kind::RefreshDesktops)
    {
        // Do not queue behind a pending Apply/Restore — those cover new apps.
        for(const Job& pending : m_pending)
        {
            if(pending.kind == Kind::ApplyAll || pending.kind == Kind::Restore)
                return;
        }
        for(const Job& pending : m_pending)
        {
            if(pending.kind == Kind::RefreshDesktops)
                return;
        }
        m_pending.append(std::move(job));
        return;
    }

    if(job.kind == Kind::RebuildDyn)
    {
        for(Job& pending : m_pending)
        {
            if(pending.kind != Kind::RebuildDyn)
                continue;
            for(const QString& path : job.dynPaths)
            {
                if(!pending.dynPaths.contains(path))
                    pending.dynPaths.append(path);
            }
            return;
        }
        m_pending.append(std::move(job));
        return;
    }

    if(job.kind == Kind::Rebuild)
    {
        for(const Job& pending : m_pending)
        {
            if(pending.kind == Kind::ApplyAll
               || pending.kind == Kind::Restore
               || pending.kind == Kind::Rebuild)
                return;
        }
        m_pending.append(std::move(job));
        return;
    }

    m_pending.append(std::move(job));
}

void IconJobQueue::kick()
{
    if(m_running || m_pending.isEmpty())
        return;
    Job job = m_pending.takeFirst();
    startJob(std::move(job));
}

void IconJobQueue::startJob(Job job)
{
    m_lock = std::make_unique<FileLock>(FileLock::defaultLockPath(), false);
    if(!m_lock->isHeld())
    {
        qInfo() << "muoto-launcher: skip job (busy) kind=" << int(job.kind);
        if(job.kind == Kind::ApplyAll)
            m_ops->emitApplyFinished(false, QStringLiteral("busy"));
        else if(job.kind == Kind::Restore)
            m_ops->emitRestoreFinished(false, QStringLiteral("busy"));
        m_lock.reset();
        kick();
        return;
    }

    m_running = std::make_unique<Job>(std::move(job));
    const Job& running = *m_running;

    switch(running.kind)
    {
    case Kind::ApplyAll:
        m_ops->runApplyIcons(running.pack, running.runPack, running.overlay);
        break;
    case Kind::Restore:
        m_ops->runRestoreIcons();
        break;
    case Kind::RefreshDesktops:
        m_ops->runRefreshNewDesktops();
        break;
    case Kind::RefreshApk:
        m_ops->runRefreshApkIcons(running.scheduleVerify);
        break;
    case Kind::RebuildDyn:
        m_ops->runRebuildDyn(running.dynPaths);
        break;
    case Kind::Rebuild:
        m_ops->runRebuild();
        break;
    }
}

void IconJobQueue::jobFinished()
{
    m_running.reset();
    m_lock.reset();
    kick();
}
