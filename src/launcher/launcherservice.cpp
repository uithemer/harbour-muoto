#include "launcherservice.h"
#include "iconjob.h"
#include "iconjobqueue.h"
#include "launchericonops.h"
#include "osupdateguard.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QMetaObject>
#include <QTimer>
#include <QDebug>

namespace {

void sendMethodReply(const QDBusMessage& message)
{
    if(message.type() != QDBusMessage::MethodCallMessage)
        return;
    if(!QDBusConnection::sessionBus().send(message.createReply()))
    {
        qWarning() << "muoto-launcher-icond: failed to send method reply:"
                   << QDBusConnection::sessionBus().lastError().message();
    }
}

const char* kLogin1Service = "org.freedesktop.login1";
const char* kLogin1Path    = "/org/freedesktop/login1";
const char* kLogin1Manager = "org.freedesktop.login1.Manager";

} // namespace

LauncherBackend::LauncherBackend(QObject* parent)
    : QObject(parent)
    , m_iconOps(LauncherIconOps::instance())
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if(!bus.connect(QString::fromLatin1(kLogin1Service),
                    QString::fromLatin1(kLogin1Path),
                    QString::fromLatin1(kLogin1Manager),
                    QStringLiteral("PrepareForShutdown"),
                    this,
                    SLOT(onPrepareForShutdown(bool))))
    {
        qWarning() << "muoto-launcher-icond: could not subscribe to PrepareForShutdown:"
                   << bus.lastError().message();
    }
}

void LauncherBackend::onPrepareForShutdown(bool active)
{
    if(!active || m_shuttingDown)
        return;
    m_shuttingDown = true;
    qInfo() << "muoto-launcher-icond: PrepareForShutdown received";
    emit prepareQuit();
}

LauncherThemesAdaptor::LauncherThemesAdaptor(LauncherBackend* backend, QObject* parent)
    : QDBusAbstractAdaptor(parent)
    , m_backend(backend)
{
    setAutoRelaySignals(false);

    IconJobQueue* queue = IconJobQueue::instance();

    // Completion is matched by request id. Back-to-back operations used to be
    // able to latch onto each other's completion; the id makes that impossible
    // now that work is queued and a reply can arrive long after the call.
    connect(queue, &IconJobQueue::jobFinished, this,
            [this](const IconJob& job, bool ok, const QString& message) {
                if(job.dbusOp.isEmpty())
                    return;
                emit OperationCompleted(job.dbusOp, ok, message);
            });

    // Queued-but-not-started is reported as indeterminate progress, which the
    // GUI already renders as "Waiting…". It replaces the client-side lock probe:
    // the daemon knows whether work is queued, the GUI could only guess by
    // sampling a lock.
    connect(queue, &IconJobQueue::jobQueued, this, [this](const IconJob& job) {
        if(!job.dbusOp.isEmpty())
            emit Progress(job.dbusOp, 0, 0);
    });

    connect(queue, &IconJobQueue::jobStarted, this, [this](const IconJob& job) {
        m_runningOp = job.dbusOp;
    });

    connect(m_backend->iconOps(), &LauncherIconOps::progress, this,
            [this, queue](int done, int total) {
                if(!m_runningOp.isEmpty())
                    emit Progress(m_runningOp, done, total);
            });
}

void LauncherThemesAdaptor::enqueueOp(const QString& op, IconJob job,
                                      const QDBusMessage& message)
{
    // Reply before the work, not after: the caller's pending call completes
    // immediately and this connection keeps serving Introspect and signal
    // traffic while the icons are rewritten.
    sendMethodReply(message);

    if(m_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        return;
    }

    job.dbusOp = op;
    QString rejection;
    if(IconJobQueue::instance()->enqueue(job, &rejection) == 0)
    {
        // Rejected on inspection rather than queued behind a drain: an empty
        // reason means it was a legitimate no-op.
        const bool ok = rejection.isEmpty();
        qInfo() << "muoto-launcher-icond:" << op << "done ok=" << ok << "msg=" << rejection;
        emit OperationCompleted(op, ok, rejection);
    }
}

void LauncherThemesAdaptor::ApplyIcons(const QString& pack, bool runPack, bool overlay,
                                       const QDBusMessage& message)
{
    IconJob job;
    job.kind = IconJob::ApplyAll;
    job.pack = pack;
    job.runPack = runPack;
    job.overlay = overlay;
    enqueueOp(QStringLiteral("ApplyIcons"), job, message);
}

void LauncherThemesAdaptor::RestoreIcons(const QDBusMessage& message)
{
    IconJob job;
    job.kind = IconJob::Restore;
    enqueueOp(QStringLiteral("RestoreIcons"), job, message);
}
