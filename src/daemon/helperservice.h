#ifndef HELPERSERVICE_H
#define HELPERSERVICE_H

#include <QObject>
#include <QTimer>
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>

#include <functional>

#include "iconapplier.h"
#include "densityenabler.h"

// HelperBackend owns the heavy worker objects + the idle-quit timer.
// The adaptor classes below hold a borrowed pointer to this backend
// and forward authorised D-Bus calls into it. Everything lives on the
// QCoreApplication main thread; ops are short enough that blocking the
// bus dispatcher between calls is fine for a single-user device.
class HelperBackend : public QObject
{
    Q_OBJECT

public:
    explicit HelperBackend(QObject* parent = nullptr);

    IconApplier& iconApplier()       { return _iconApplier; }
    DensityEnabler& densityEnabler() { return _densityEnabler; }

    // Re-arm the 30 s idle timer on every method dispatch. When the
    // timer fires, the daemon emits idleQuit() and main() asks the
    // QCoreApplication to exit; systemd / dbus-daemon spin us back up
    // on the next call.
    void resetIdleTimer();

    // True once login1.Manager.PrepareForShutdown(true) has fired. The
    // adaptors check this gate at method entry and refuse to dispatch
    // new work; the broadcast OperationCompleted(op, false, "shutting
    // down") still goes out so the GUI's busy state drains. We quit
    // the QCoreApplication ~150 ms later (chained off idleQuit()) so
    // the broadcast has room to flush.
    bool shuttingDown() const { return _shuttingDown; }

signals:
    void idleQuit();

private slots:
    void onIdleTimeout();
    void onPrepareForShutdown(bool active);

private:
    IconApplier    _iconApplier;
    DensityEnabler _densityEnabler;
    QTimer         _idleTimer;
    bool           _shuttingDown = false;
};

// One QDBusAbstractAdaptor per logical interface. Each adaptor exposes
// every method as a public slot taking the call's QDBusMessage as the
// last arg so the polkit subject can be derived from message.service().
class ThemesAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.uithemer.UiThemer1.Themes")

public:
    ThemesAdaptor(HelperBackend* backend, QObject* parent);

public slots:
    void ApplyIcons(const QString& pack, bool runPack, bool overlay,
                    const QDBusMessage& message);
    void RestoreIcons(const QDBusMessage& message);
    void RefreshOriginals(const QDBusMessage& message);
    void DensityEnable(const QDBusMessage& message);

signals:
    void OperationCompleted(const QString& op, bool ok,
                            const QString& message);
    void Progress(const QString& op, int done, int total);

private:
    // Returns true if authorised. On denial, broadcasts
    // OperationCompleted(op, false, "polkit denied") so the GUI's
    // busy state clears even though the method body never runs.
    bool authorize(const QDBusMessage& message, const QString& op);

    // Hook IconApplier's per-op completion signal once for the
    // duration of one op, broadcast OperationCompleted, then
    // disconnect. The lambda passed in `start` is what actually
    // invokes the relevant IconApplier slot; `doneSignal` is the
    // IconApplier::xxx member-pointer that the op fires on success
    // (and on logged-but-non-fatal failure -- IconApplier always
    // emits its done signal so QML/HelperClient's busy spinner can
    // drain).
    void runIconOpVoid(const QString& op,
                       std::function<void(IconApplier&)> start,
                       void (IconApplier::*doneSignal)());

    HelperBackend* _backend;
};

class PacksAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.uithemer.UiThemer1.Packs")

public:
    PacksAdaptor(HelperBackend* backend, QObject* parent);

public slots:
    void UninstallPack(const QString& rpmName, const QDBusMessage& message);

signals:
    void OperationCompleted(const QString& op, bool ok,
                            const QString& message);

private:
    bool authorize(const QDBusMessage& message, const QString& op);
    HelperBackend* _backend;
};

#endif // HELPERSERVICE_H
