#include "launcherservice.h"
#include "launchericonops.h"
#include "osupdateguard.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QMetaObject>
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
}

void LauncherThemesAdaptor::runIconOpVoid(const QString& op,
                                          std::function<void(LauncherIconOps&)> start,
                                          void (LauncherIconOps::*doneSignal)(bool, const QString&))
{
    LauncherIconOps* ops = m_backend->iconOps();

    auto* conn = new QMetaObject::Connection;
    *conn = connect(ops, doneSignal, this,
                    [this, op, conn](bool ok, const QString& message) {
                        emit OperationCompleted(op, ok, message);
                        QObject::disconnect(*conn);
                        delete conn;
                    });

    start(*ops);
}

void LauncherThemesAdaptor::ApplyIcons(const QString& pack, bool runPack, bool overlay,
                                       const QDBusMessage& message)
{
    const QString op = QStringLiteral("ApplyIcons");
    if(m_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        sendMethodReply(message);
        return;
    }
    if(OsUpdateGuard::running())
    {
        emit OperationCompleted(op, false, QStringLiteral("upgrade in progress"));
        sendMethodReply(message);
        return;
    }

    runIconOpVoid(op, [pack, runPack, overlay](LauncherIconOps& o) {
                  o.applyIcons(pack, runPack, overlay); },
                  &LauncherIconOps::applied);
    sendMethodReply(message);
}

void LauncherThemesAdaptor::RestoreIcons(const QDBusMessage& message)
{
    const QString op = QStringLiteral("RestoreIcons");
    if(m_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        sendMethodReply(message);
        return;
    }

    runIconOpVoid(op, [](LauncherIconOps& o) { o.restoreIcons(); },
                  &LauncherIconOps::restored);
    sendMethodReply(message);
}
