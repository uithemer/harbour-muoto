#include "helperservice.h"

#include <QProcess>
#include <QStringList>
#include <QDBusConnection>
#include <QDebug>

namespace
{
    const int kIdleTimeoutMs = 30 * 1000;
    const int kShutdownDrainMs = 150;

    void sendMethodReply(const QDBusMessage &message)
    {
        if(message.type() != QDBusMessage::MethodCallMessage)
            return;
        if(!QDBusConnection::systemBus().send(message.createReply()))
        {
            qWarning() << "muoto-helperd: failed to send method reply:"
                       << QDBusConnection::systemBus().lastError().message();
        }
    }

    const char *kLogin1Service = "org.freedesktop.login1";
    const char *kLogin1Path    = "/org/freedesktop/login1";
    const char *kLogin1Manager = "org.freedesktop.login1.Manager";

}

HelperBackend::HelperBackend(QObject *parent) : QObject(parent)
{
    _idleTimer.setSingleShot(true);
    _idleTimer.setInterval(kIdleTimeoutMs);
    connect(&_idleTimer, &QTimer::timeout,
            this, &HelperBackend::onIdleTimeout);
    _idleTimer.start();

    QDBusConnection bus = QDBusConnection::systemBus();
    if(!bus.connect(QString::fromLatin1(kLogin1Service),
                    QString::fromLatin1(kLogin1Path),
                    QString::fromLatin1(kLogin1Manager),
                    QStringLiteral("PrepareForShutdown"),
                    this,
                    SLOT(onPrepareForShutdown(bool))))
    {
        qWarning() << "muoto-helperd: could not subscribe to"
                   << "login1.Manager.PrepareForShutdown:"
                   << bus.lastError().message();
    }
}

void HelperBackend::resetIdleTimer()
{
    if(_idleSuspendCount == 0)
        _idleTimer.start();
}

void HelperBackend::suspendIdleTimer()
{
  ++_idleSuspendCount;
    _idleTimer.stop();
}

void HelperBackend::resumeIdleTimer()
{
    if(_idleSuspendCount > 0)
        --_idleSuspendCount;
    if(_idleSuspendCount == 0 && !_shuttingDown)
        _idleTimer.start();
}

void HelperBackend::onIdleTimeout()
{
    if(_idleSuspendCount > 0)
    {
        _idleTimer.start();
        return;
    }
    qInfo() << "muoto-helperd: idle timeout, quitting";
    emit idleQuit();
}

void HelperBackend::onPrepareForShutdown(bool active)
{
    if(!active)
        return;
    if(_shuttingDown)
        return;
    _shuttingDown = true;
    qInfo() << "muoto-helperd: PrepareForShutdown received, draining";
    _idleTimer.stop();
    QTimer::singleShot(kShutdownDrainMs, this,
                       [this]() { emit idleQuit(); });
}

ThemesAdaptor::ThemesAdaptor(HelperBackend *backend, QObject *parent)
    : QDBusAbstractAdaptor(parent), _backend(backend)
{
    setAutoRelaySignals(false);
}

bool ThemesAdaptor::authorize(const QDBusMessage &message, const QString &op)
{
    Q_UNUSED(message);
    Q_UNUSED(op);
    return true;
}

void ThemesAdaptor::DensityEnable(const QDBusMessage &message)
{
    const QString op = QStringLiteral("DensityEnable");
    if (_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        sendMethodReply(message);
        return;
    }
    if (!authorize(message, op))
    {
        sendMethodReply(message);
        return;
    }
    _backend->resetIdleTimer();
    DensityEnabler &density = _backend->densityEnabler();
    auto *enabledConn = new QMetaObject::Connection;
    auto *errorConn = new QMetaObject::Connection;
    auto finish = [this, op, enabledConn, errorConn](bool ok, const QString &msg)
    {
        emit OperationCompleted(op, ok, msg);
        QObject::disconnect(*enabledConn);
        QObject::disconnect(*errorConn);
        delete enabledConn;
        delete errorConn;
        _backend->resetIdleTimer();
    };
    *enabledConn = connect(&density, &DensityEnabler::enabled, this,
                           [finish]() { finish(true, QString()); });
    *errorConn = connect(&density, &DensityEnabler::error, this,
                         [finish](const QString &message) { finish(false, message); });
    density.ensureEnabled();
    sendMethodReply(message);
}

PacksAdaptor::PacksAdaptor(HelperBackend *backend, QObject *parent)
    : QDBusAbstractAdaptor(parent), _backend(backend)
{
    setAutoRelaySignals(false);
}

bool PacksAdaptor::authorize(const QDBusMessage &message, const QString &op)
{
    Q_UNUSED(message);
    Q_UNUSED(op);
    return true;
}

void PacksAdaptor::UninstallPack(const QString &rpmName,
                                 const QDBusMessage &message)
{
    const QString op = QStringLiteral("UninstallPack");
    if (_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        sendMethodReply(message);
        return;
    }
    if (!authorize(message, op))
    {
        sendMethodReply(message);
        return;
    }

    _backend->resetIdleTimer();

    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(QStringLiteral("rpm"),
            QStringList() << QStringLiteral("-e") << rpmName);
    if (!p.waitForStarted(5000))
    {
        emit OperationCompleted(op, false,
                                QStringLiteral("failed to start rpm"));
        sendMethodReply(message);
        return;
    }
    if (!p.waitForFinished(60000))
    {
        p.kill();
        emit OperationCompleted(op, false,
                                QStringLiteral("rpm timed out"));
        sendMethodReply(message);
        return;
    }
    const bool ok = (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0);
    emit OperationCompleted(op, ok,
                            ok ? rpmName
                               : QStringLiteral("rpm exited %1").arg(p.exitCode()));
    _backend->resetIdleTimer();
    sendMethodReply(message);
}
