#include "helperservice.h"

#include <QProcess>
#include <QStringList>
#include <QMetaObject>
#include <QDBusConnection>
#include <QDebug>

namespace
{
    const int kIdleTimeoutMs = 30 * 1000;
    const int kShutdownDrainMs = 150;

    const char *kLogin1Service = "org.freedesktop.login1";
    const char *kLogin1Path    = "/org/freedesktop/login1";
    const char *kLogin1Manager = "org.freedesktop.login1.Manager";
}

// =====================================================================
// HelperBackend
// =====================================================================

HelperBackend::HelperBackend(QObject *parent) : QObject(parent)
{
    _idleTimer.setSingleShot(true);
    _idleTimer.setInterval(kIdleTimeoutMs);
    connect(&_idleTimer, &QTimer::timeout,
            this, &HelperBackend::onIdleTimeout);
    _idleTimer.start();

    // Subscribe to systemd-logind's pre-shutdown / pre-reboot broadcast.
    // When the kernel is moments away from killing every process,
    // accepting a new ApplyIcons that we cannot complete would leave
    // the manifest and the .desktop files out of sync. The slot flips
    // _shuttingDown so adaptor methods refuse new work, then schedules
    // QCoreApplication::quit() after a short drain window so any in-
    // flight OperationCompleted broadcasts make it onto the bus before
    // we go away.
    QDBusConnection bus = QDBusConnection::systemBus();
    if(!bus.connect(QString::fromLatin1(kLogin1Service),
                    QString::fromLatin1(kLogin1Path),
                    QString::fromLatin1(kLogin1Manager),
                    QStringLiteral("PrepareForShutdown"),
                    this,
                    SLOT(onPrepareForShutdown(bool))))
    {
        qWarning() << "uithemer-helperd: could not subscribe to"
                   << "login1.Manager.PrepareForShutdown:"
                   << bus.lastError().message();
    }
}

void HelperBackend::resetIdleTimer()
{
    _idleTimer.start();
}

void HelperBackend::onIdleTimeout()
{
    qInfo() << "uithemer-helperd: idle timeout, quitting";
    emit idleQuit();
}

void HelperBackend::onPrepareForShutdown(bool active)
{
    if(!active)
    {
        // logind also fires PrepareForShutdown(false) when a shutdown
        // is cancelled. We never re-arm because by the time we get
        // here new clients would have already given up.
        return;
    }
    if(_shuttingDown)
        return;
    _shuttingDown = true;
    qInfo() << "uithemer-helperd: PrepareForShutdown received, draining";
    _idleTimer.stop();
    // Quit via the same idleQuit signal main() already wires to
    // QCoreApplication::quit, with a small drain so any pending
    // OperationCompleted("...", false, "shutting down") broadcasts
    // dispatched from adaptor methods reach subscribers.
    QTimer::singleShot(kShutdownDrainMs, this,
                       [this]() { emit idleQuit(); });
}

// =====================================================================
// ThemesAdaptor
// =====================================================================

ThemesAdaptor::ThemesAdaptor(HelperBackend *backend, QObject *parent)
    : QDBusAbstractAdaptor(parent), _backend(backend)
{
    setAutoRelaySignals(false);
    // IconApplier::progress -> broadcast Progress("<currentOp>", ...).
    // We don't know the current op string here, so we forward with a
    // generic tag; HelperClient demuxes by op string. Op-specific
    // progress is bridged inside runIconOp via a per-call lambda.
}

bool ThemesAdaptor::authorize(const QDBusMessage &message, const QString &op)
{
    // Auth is enforced at the bus-policy layer
    // (/etc/dbus-1/system.d/org.uithemer.UiThemer1.conf): only root may
    // own the well-known name, and dbus-daemon rejects sends from any
    // user the policy has not allow-listed. Once a message reaches us
    // here, the caller has already cleared that gate, so we accept
    // unconditionally.
    Q_UNUSED(message);
    Q_UNUSED(op);
    return true;
}

void ThemesAdaptor::runIconOpVoid(const QString &op,
                                  std::function<void(IconApplier &)> start,
                                  void (IconApplier::*doneSignal)())
{
    _backend->resetIdleTimer();
    IconApplier &applier = _backend->iconApplier();

    const int epoch = _backend->nextIconOpEpoch();
    auto *conn = new QMetaObject::Connection;
    *conn = connect(&applier, doneSignal, this, [this, op, epoch, conn]()
                    {
        emit OperationCompleted(op, true, QString::number(epoch));
        QObject::disconnect(*conn);
        delete conn;
        _backend->resetIdleTimer(); });

    auto *progressConn = new QMetaObject::Connection;
    *progressConn = connect(&applier, &IconApplier::progress, this,
                            [this, op](int done, int total)
                            {
                                emit Progress(op, done, total);
                            });
    // Disconnect progress when done. Schedule it via a one-shot
    // single-shot timer chained to the done signal. Simpler: also
    // disconnect when we get the done signal. Use a second slot:
    auto *progressOff = new QMetaObject::Connection;
    *progressOff = connect(&applier, doneSignal, this,
                           [progressConn, progressOff]()
                           {
                               QObject::disconnect(*progressConn);
                               QObject::disconnect(*progressOff);
                               delete progressConn;
                               delete progressOff;
                           });

    start(applier);
}

void ThemesAdaptor::ApplyIcons(const QString &pack, bool overlay,
                               const QDBusMessage &message)
{
    const QString op = QStringLiteral("ApplyIcons");
    if (_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        return;
    }
    if (!authorize(message, op))
        return;
    runIconOpVoid(op, [pack, overlay](IconApplier &a)
                  { a.applyIcons(pack, overlay); }, &IconApplier::applied);
}

void ThemesAdaptor::RestoreIcons(const QDBusMessage &message)
{
    const QString op = QStringLiteral("RestoreIcons");
    if (_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        return;
    }
    if (!authorize(message, op))
        return;
    runIconOpVoid(op, [](IconApplier &a)
                  { a.restoreIcons(); }, &IconApplier::restored);
}

void ThemesAdaptor::RefreshOriginals(const QDBusMessage &message)
{
    const QString op = QStringLiteral("RefreshOriginals");
    if (_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        return;
    }
    if (!authorize(message, op))
        return;
    runIconOpVoid(op, [](IconApplier &a)
                  { a.refreshOriginals(); }, &IconApplier::originalsRefreshed);
}

void ThemesAdaptor::DensityEnable(const QDBusMessage &message)
{
    const QString op = QStringLiteral("DensityEnable");
    if (_backend->shuttingDown())
    {
        emit OperationCompleted(op, false, QStringLiteral("shutting down"));
        return;
    }
    if (!authorize(message, op))
        return;
    _backend->resetIdleTimer();
    DensityEnabler &density = _backend->densityEnabler();
    auto *conn = new QMetaObject::Connection;
    *conn = connect(&density, &DensityEnabler::enabled, this, [this, op, conn]()
                    {
        emit OperationCompleted(op, true, QString());
        QObject::disconnect(*conn);
        delete conn;
        _backend->resetIdleTimer(); });
    density.ensureEnabled();
}

// =====================================================================
// PacksAdaptor
// =====================================================================

PacksAdaptor::PacksAdaptor(HelperBackend *backend, QObject *parent)
    : QDBusAbstractAdaptor(parent), _backend(backend)
{
    setAutoRelaySignals(false);
}

bool PacksAdaptor::authorize(const QDBusMessage &message, const QString &op)
{
    // Bus-policy enforced -- see ThemesAdaptor::authorize().
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
        return;
    }
    if (!authorize(message, op))
        return;

    _backend->resetIdleTimer();

    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(QStringLiteral("rpm"),
            QStringList() << QStringLiteral("-e") << rpmName);
    if (!p.waitForStarted(5000))
    {
        emit OperationCompleted(op, false,
                                QStringLiteral("failed to start rpm"));
        return;
    }
    if (!p.waitForFinished(60000))
    {
        p.kill();
        emit OperationCompleted(op, false,
                                QStringLiteral("rpm timed out"));
        return;
    }
    const bool ok = (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0);
    emit OperationCompleted(op, ok,
                            ok ? rpmName
                               : QStringLiteral("rpm exited %1").arg(p.exitCode()));
    _backend->resetIdleTimer();
}

