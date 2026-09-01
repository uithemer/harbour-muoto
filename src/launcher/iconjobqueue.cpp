#include "iconjobqueue.h"
#include "filelock.h"
#include "iconpaths.h"
#include "launchericonops.h"
#include "osupdateguard.h"

#include <QDebug>
#include <QDir>

namespace {

const int kLockRetryMs = 500;
// Another process (helperd, during a density unlock or a pack removal) can hold
// the sentinel. Waiting it out is right -- a user's apply should not fail
// because helperd was mid-operation -- but not forever.
const int kLockGiveUpMs = 120 * 1000;

bool isFullOp(IconJob::Kind kind)
{
    return kind == IconJob::ApplyAll || kind == IconJob::Restore;
}

} // namespace

IconJobQueue* IconJobQueue::instance()
{
    static auto* s_instance = new IconJobQueue;
    return s_instance;
}

IconJobQueue::IconJobQueue(QObject* parent)
    : QObject(parent)
{
    m_lockRetry.setInterval(kLockRetryMs);
    m_lockRetry.setSingleShot(true);
    connect(&m_lockRetry, &QTimer::timeout, this, &IconJobQueue::startNext);

    connect(LauncherIconOps::instance(), &LauncherIconOps::jobFinished,
            this, &IconJobQueue::onJobFinished);
}

void IconJobQueue::beginSelfWrite(const QString& key, const QString& value)
{
    m_selfWriteKey = key;
    m_selfWriteValue = value;
}

void IconJobQueue::endSelfWrite()
{
    m_selfWriteKey.clear();
    m_selfWriteValue.clear();
}

bool IconJobQueue::isSelfWrite(const QString& key, const QString& value) const
{
    // Matched on key *and* value, never on a time window. ThemeWork deliberately
    // pulses the dyn flags (writes the opposite value, then the wanted one) to
    // force a dconf watch to fire after a restore; a time-based guard would
    // swallow that and the dynamic clock would silently stay off.
    return !m_selfWriteKey.isEmpty() && key == m_selfWriteKey && value == m_selfWriteValue;
}

quint64 IconJobQueue::enqueue(const IconJob& job, QString* rejection)
{
    const auto reject = [rejection](const QString& why) -> quint64 {
        if(rejection)
            *rejection = why;
        return 0;
    };

    // Static preconditions answer now. Deferring them into the job would also
    // break pipeline P2, which sends an unknown pack and greps the completion
    // out of a short dbus-monitor window.
    if(job.kind == IconJob::ApplyAll)
    {
        if(job.pack.isEmpty() || job.pack == QLatin1String("default"))
            return reject(QString());
        if(!QDir(IconPaths::packDir(job.pack)).exists())
            return reject(QStringLiteral("pack not found"));
    }

    IconJob queued = job;
    queued.id = m_nextId++;

    if(coalesceInto(queued))
    {
        // Folded into something already pending; report the surviving id.
        return m_pending.isEmpty() ? queued.id : m_pending.last().id;
    }

    m_pending.append(queued);
    emit jobQueued(queued);
    schedule();
    return queued.id;
}

bool IconJobQueue::coalesceInto(const IconJob& job)
{
    for(int i = 0; i < m_pending.size(); ++i)
    {
        IconJob& pending = m_pending[i];
        if(pending.kind != job.kind)
            continue;

        switch(job.kind)
        {
        case IconJob::ApplyAll:
            // Latest arguments win: an older pending apply of a different pack
            // would only be overwritten a moment later anyway.
            pending.pack = job.pack;
            pending.runPack = job.runPack;
            pending.overlay = job.overlay;
            pending.dbusOp = job.dbusOp.isEmpty() ? pending.dbusOp : job.dbusOp;
            return true;
        case IconJob::RefreshDesktops:
            for(const QString& p : job.paths)
            {
                if(!pending.paths.contains(p))
                    pending.paths.append(p);
            }
            return true;
        case IconJob::RefreshApk:
        case IconJob::Rebuild:
        case IconJob::RebuildDyn:
            return true;
        case IconJob::Restore:
            // Never folded and never reordered against an apply.
            return false;
        }
    }

    // A plain rebuild is redundant once a full operation is queued behind it:
    // both end in their own rebuild. uninstallPack writes the dyn flags and then
    // calls RestoreIcons, which used to provoke exactly this wasted pass.
    if(job.kind == IconJob::Rebuild || job.kind == IconJob::RebuildDyn)
    {
        for(const IconJob& pending : m_pending)
        {
            if(isFullOp(pending.kind))
                return true;
        }
    }

    if(isFullOp(job.kind))
    {
        for(int i = m_pending.size() - 1; i >= 0; --i)
        {
            if(m_pending.at(i).kind == IconJob::Rebuild
               || m_pending.at(i).kind == IconJob::RebuildDyn)
            {
                m_pending.removeAt(i);
            }
        }
    }

    return false;
}

void IconJobQueue::schedule()
{
    if(busy() || m_pending.isEmpty() || m_lockRetry.isActive())
        return;
    startNext();
}

void IconJobQueue::startNext()
{
    if(busy() || m_pending.isEmpty())
        return;

    if(m_lock.isNull())
    {
        m_lock.reset(new FileLock(FileLock::defaultLockPath(), false));
        if(!m_lock->isHeld())
        {
            m_lock.reset();
            m_lockWaitedMs += kLockRetryMs;
            if(m_lockWaitedMs >= kLockGiveUpMs)
            {
                qWarning() << "muoto-launcher: icon-ops.lock held by another process for"
                           << m_lockWaitedMs << "ms; dropping" << m_pending.size() << "job(s)";
                const QList<IconJob> dropped = m_pending;
                m_pending.clear();
                m_lockWaitedMs = 0;
                for(const IconJob& job : dropped)
                    emit jobFinished(job, false, QStringLiteral("icon operations are busy"));
                return;
            }
            m_lockRetry.start();
            return;
        }
        m_lockWaitedMs = 0;
    }

    IconJob job = m_pending.takeFirst();

    // Volatile guards are re-tested here rather than at enqueue: the state can
    // change while a job waits behind a drain.
    QString guard;
    if(OsUpdateGuard::running())
        guard = QStringLiteral("upgrade in progress");

    if(!guard.isEmpty())
    {
        emit jobFinished(job, false, guard);
        if(m_pending.isEmpty())
            m_lock.reset();
        else
            schedule();
        return;
    }

    m_current = job;
    emit jobStarted(job);
    LauncherIconOps::instance()->startJob(job);
}

void IconJobQueue::onJobFinished(bool ok, const QString& message)
{
    const IconJob done = m_current;
    m_current = IconJob();

    emit jobFinished(done, ok, message);

    if(m_pending.isEmpty())
    {
        // Drain over: only now does the lock go, so a shell caller that saw it
        // taken and then released knows its own request has run.
        m_lock.reset();
        return;
    }
    startNext();
}
