#include "helperservice.h"
#include "auth.h"

#include <QProcess>
#include <QStringList>
#include <QMetaObject>
#include <QDebug>

namespace
{
    const int kIdleTimeoutMs = 30 * 1000;

    const char* kManageThemes  = "org.uithemer.manage-themes";
    const char* kManagePacks   = "org.uithemer.manage-packs";
    const char* kManageSystem  = "org.uithemer.manage-system-services";
}

// =====================================================================
// HelperBackend
// =====================================================================

HelperBackend::HelperBackend(QObject* parent) : QObject(parent)
{
    _idleTimer.setSingleShot(true);
    _idleTimer.setInterval(kIdleTimeoutMs);
    connect(&_idleTimer, &QTimer::timeout,
            this, &HelperBackend::onIdleTimeout);
    _idleTimer.start();
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

// =====================================================================
// ThemesAdaptor
// =====================================================================

ThemesAdaptor::ThemesAdaptor(HelperBackend* backend, QObject* parent)
    : QDBusAbstractAdaptor(parent), _backend(backend)
{
    setAutoRelaySignals(false);
    // IconApplier::progress -> broadcast Progress("<currentOp>", ...).
    // We don't know the current op string here, so we forward with a
    // generic tag; HelperClient demuxes by op string. Op-specific
    // progress is bridged inside runIconOp via a per-call lambda.
}

bool ThemesAdaptor::authorize(const QDBusMessage& message, const QString& op)
{
    if(UiThemerAuth::isAuthorized(message, kManageThemes))
        return true;
    qWarning() << "ThemesAdaptor: polkit denied" << op;
    emit OperationCompleted(op, false, QStringLiteral("polkit denied"));
    return false;
}

void ThemesAdaptor::runIconOpVoid(const QString& op,
                                  std::function<void(IconApplier&)> start,
                                  void (IconApplier::*doneSignal)())
{
    _backend->resetIdleTimer();
    IconApplier& applier = _backend->iconApplier();

    auto* conn = new QMetaObject::Connection;
    *conn = connect(&applier, doneSignal, this, [this, op, conn]() {
        emit OperationCompleted(op, true, QString());
        QObject::disconnect(*conn);
        delete conn;
        _backend->resetIdleTimer();
    });

    auto* progressConn = new QMetaObject::Connection;
    *progressConn = connect(&applier, &IconApplier::progress, this,
                            [this, op](int done, int total) {
        emit Progress(op, done, total);
    });
    // Disconnect progress when done. Schedule it via a one-shot
    // single-shot timer chained to the done signal. Simpler: also
    // disconnect when we get the done signal. Use a second slot:
    auto* progressOff = new QMetaObject::Connection;
    *progressOff = connect(&applier, doneSignal, this,
                           [progressConn, progressOff]() {
        QObject::disconnect(*progressConn);
        QObject::disconnect(*progressOff);
        delete progressConn;
        delete progressOff;
    });

    start(applier);
}

void ThemesAdaptor::ApplyIcons(const QString& pack, bool overlay,
                               const QDBusMessage& message)
{
    const QString op = QStringLiteral("ApplyIcons");
    if(!authorize(message, op))
        return;
    runIconOpVoid(op,
                  [pack, overlay](IconApplier& a) { a.applyIcons(pack, overlay); },
                  &IconApplier::applied);
}

void ThemesAdaptor::RestoreIcons(const QDBusMessage& message)
{
    const QString op = QStringLiteral("RestoreIcons");
    if(!authorize(message, op))
        return;
    runIconOpVoid(op,
                  [](IconApplier& a) { a.restoreIcons(); },
                  &IconApplier::restored);
}

void ThemesAdaptor::ReassertIcons(const QDBusMessage& message)
{
    const QString op = QStringLiteral("ReassertIcons");
    if(!authorize(message, op))
        return;
    runIconOpVoid(op,
                  [](IconApplier& a) { a.reassertCurrentTheme(); },
                  &IconApplier::reasserted);
}

void ThemesAdaptor::RefreshOriginals(const QDBusMessage& message)
{
    const QString op = QStringLiteral("RefreshOriginals");
    if(!authorize(message, op))
        return;
    runIconOpVoid(op,
                  [](IconApplier& a) { a.refreshOriginals(); },
                  &IconApplier::originalsRefreshed);
}

void ThemesAdaptor::ThemeNewDesktops(const QDBusMessage& message)
{
    const QString op = QStringLiteral("ThemeNewDesktops");
    if(!authorize(message, op))
        return;
    // newDesktopsThemed(int count) is the completion signal here.
    _backend->resetIdleTimer();
    IconApplier& applier = _backend->iconApplier();
    auto* conn = new QMetaObject::Connection;
    *conn = connect(&applier, &IconApplier::newDesktopsThemed, this,
                    [this, op, conn](int count) {
        emit OperationCompleted(op, true,
                                QString::number(count));
        QObject::disconnect(*conn);
        delete conn;
        _backend->resetIdleTimer();
    });
    applier.themeNewDesktops();
}

void ThemesAdaptor::DensityEnable(const QDBusMessage& message)
{
    const QString op = QStringLiteral("DensityEnable");
    if(!authorize(message, op))
        return;
    _backend->resetIdleTimer();
    DensityEnabler& density = _backend->densityEnabler();
    auto* conn = new QMetaObject::Connection;
    *conn = connect(&density, &DensityEnabler::enabled, this, [this, op, conn]() {
        emit OperationCompleted(op, true, QString());
        QObject::disconnect(*conn);
        delete conn;
        _backend->resetIdleTimer();
    });
    density.ensureEnabled();
}

// =====================================================================
// PacksAdaptor
// =====================================================================

PacksAdaptor::PacksAdaptor(HelperBackend* backend, QObject* parent)
    : QDBusAbstractAdaptor(parent), _backend(backend)
{
    setAutoRelaySignals(false);
}

bool PacksAdaptor::authorize(const QDBusMessage& message, const QString& op)
{
    if(UiThemerAuth::isAuthorized(message, kManagePacks))
        return true;
    qWarning() << "PacksAdaptor: polkit denied" << op;
    emit OperationCompleted(op, false, QStringLiteral("polkit denied"));
    return false;
}

void PacksAdaptor::UninstallPack(const QString& rpmName,
                                 const QDBusMessage& message)
{
    Q_UNUSED(_backend);
    const QString op = QStringLiteral("UninstallPack");
    if(!authorize(message, op))
        return;

    _backend->resetIdleTimer();

    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(QStringLiteral("rpm"),
            QStringList() << QStringLiteral("-e") << rpmName);
    if(!p.waitForStarted(5000))
    {
        emit OperationCompleted(op, false,
            QStringLiteral("failed to start rpm"));
        return;
    }
    if(!p.waitForFinished(60000))
    {
        p.kill();
        emit OperationCompleted(op, false,
            QStringLiteral("rpm timed out"));
        return;
    }
    const bool ok = (p.exitStatus() == QProcess::NormalExit
                     && p.exitCode() == 0);
    emit OperationCompleted(op, ok,
        ok ? rpmName
           : QStringLiteral("rpm exited %1").arg(p.exitCode()));
    _backend->resetIdleTimer();
}

// =====================================================================
// SystemServicesAdaptor
// =====================================================================

SystemServicesAdaptor::SystemServicesAdaptor(HelperBackend* backend,
                                             QObject* parent)
    : QDBusAbstractAdaptor(parent), _backend(backend)
{
    setAutoRelaySignals(false);
}

bool SystemServicesAdaptor::authorize(const QDBusMessage& message,
                                      const QString& op)
{
    if(UiThemerAuth::isAuthorized(message, kManageSystem))
        return true;
    qWarning() << "SystemServicesAdaptor: polkit denied" << op;
    emit OperationCompleted(op, false, QStringLiteral("polkit denied"));
    return false;
}

void SystemServicesAdaptor::HideIcon(const QDBusMessage& message)
{
    const QString op = QStringLiteral("HideIcon");
    if(!authorize(message, op))
        return;
    _backend->resetIdleTimer();
    const bool ok = _backend->themePackOps().hideIcon();
    emit OperationCompleted(op, ok,
        ok ? QString()
           : QStringLiteral("hideIcon failed"));
}

void SystemServicesAdaptor::SetAutoupdate(bool enabled,
                                          const QDBusMessage& message)
{
    const QString op = QStringLiteral("SetAutoupdate");
    if(!authorize(message, op))
        return;
    _backend->resetIdleTimer();
    const bool ok = _backend->themePackOps().setAutoupdate(enabled);
    emit OperationCompleted(op, ok,
        ok ? QString()
           : QStringLiteral("setAutoupdate failed"));
}

void SystemServicesAdaptor::SetServiceSu(bool enabled,
                                         const QDBusMessage& message)
{
    const QString op = QStringLiteral("SetServiceSu");
    if(!authorize(message, op))
        return;
    _backend->resetIdleTimer();
    const bool ok = _backend->themePackOps().setServiceSu(enabled);
    emit OperationCompleted(op, ok,
        ok ? QString()
           : QStringLiteral("setServiceSu failed"));
}

void SystemServicesAdaptor::ApplyHours(const QString& hours,
                                       const QDBusMessage& message)
{
    const QString op = QStringLiteral("ApplyHours");
    if(!authorize(message, op))
        return;
    _backend->resetIdleTimer();
    const bool ok = _backend->themePackOps().applyHours(hours);
    emit OperationCompleted(op, ok,
        ok ? hours
           : QStringLiteral("applyHours failed"));
}
