#ifndef ICONJOBQUEUE_H
#define ICONJOBQUEUE_H

#include "filelock.h"
#include "iconjob.h"
#include "muotolauncherglobal.h"

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QTimer>

// Serialises every icon operation in the daemon.
//
// The old design ran each operation wherever its trigger fired, and used nested
// event loops for the Lipstick waits, so a D-Bus apply, the desktop watcher and
// the 60 s dynamic tick could all be part-way through at once. FileLock could
// not help: they shared a process. That overlap is what let a write land on an
// entry another operation had renamed aside.
//
// Here exactly one job runs at a time and callers never block. The flock is held
// across the whole drain rather than per job, which is load-bearing for the
// shell callers: harbour-muoto-update-icons and the repair oneshot decide an
// operation finished by watching that lock, so a per-job lock would let them
// observe someone else's and report success before their own request ran.
class MUOTO_LAUNCHER_EXPORT IconJobQueue : public QObject
{
    Q_OBJECT

public:
    static IconJobQueue* instance();

    // Returns 0 when the job was rejected outright (see rejected()), otherwise
    // the request id. Cheap, decidable-on-inspection failures are answered here
    // rather than queued: there is no sense making a caller wait out a drain to
    // be told the pack does not exist.
    quint64 enqueue(const IconJob& job, QString* rejection = nullptr);

    bool busy() const { return m_current.id != 0; }

    // Set while a job is applying its own dconf writes, so the watches those
    // writes trigger do not enqueue a rebuild of work the job is already doing.
    void beginSelfWrite(const QString& key, const QString& value);
    void endSelfWrite();
    bool isSelfWrite(const QString& key, const QString& value) const;

signals:
    void jobStarted(const IconJob& job);
    void jobFinished(const IconJob& job, bool ok, const QString& message);
    // Emitted when a job is queued but not yet running, so a caller can tell
    // "waiting behind a drain" from "the daemon died".
    void jobQueued(const IconJob& job);

private:
    explicit IconJobQueue(QObject* parent = nullptr);

    void schedule();
    void startNext();
    void onJobFinished(bool ok, const QString& message);
    bool coalesceInto(const IconJob& job);

    QList<IconJob> m_pending;
    IconJob m_current;
    quint64 m_nextId = 1;

    // Held from the queue going non-empty to it going empty.
    QScopedPointer<FileLock> m_lock;
    QTimer m_lockRetry;
    int m_lockWaitedMs = 0;

    QString m_selfWriteKey;
    QString m_selfWriteValue;
};

#endif // ICONJOBQUEUE_H
