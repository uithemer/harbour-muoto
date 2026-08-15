#ifndef HELPERCLIENT_H
#define HELPERCLIENT_H

#include <QObject>
#include <QString>
#include <QVariantList>

class QTimer;
class QQmlEngine;
class QJSEngine;

// HelperClient: process-wide Q_OBJECT facade for privileged theme ops.
// Exposed to QML as the `Helper` singleton (qmlRegisterSingletonType)
// and shared with ThemePackModel via HelperClient::instance().
// Singleton because each instance opens its own D-Bus match rule, and
// we used to create up to six of them — wasteful and noisy.
//
// Icons go to session org.muoto.Launcher1.Themes; density unlock and
// pack uninstall go to system org.muoto.Muoto1. All slots return
// immediately. Real success / failure comes through the matching Qt
// signal; if the daemon is not available, error() fires so the GUI's
// busy spinner clears.
//
// Nothing here may block the QML thread: calls go out as plain
// QDBusMessage + asyncCall (a QDBusInterface would introspect
// synchronously first), and the session daemon is started detached and
// polled rather than waited on.
class HelperClient : public QObject
{
    Q_OBJECT

public:
    // Process-wide accessor. Lazily constructs on first call; the
    // returned pointer is owned by the C++ side and outlives every
    // QObject parent (callers MUST NOT delete it).
    static HelperClient* instance();

    // Factory for qmlRegisterSingletonType. Forwards to instance()
    // and pins QQmlEngine::CppOwnership so the engine cannot GC us.
    static QObject* qmlSingleton(QQmlEngine* engine, QJSEngine* scriptEngine);

    ~HelperClient() override;

public slots:
    // -- session org.muoto.Launcher1.Themes --
    void applyIcons(const QString& pack, bool runPack, bool overlay);
    void restoreIcons();
    // -- system org.muoto.Muoto1.Themes (bus-policy gated) --
    void densityEnable();

    // -- system org.muoto.Muoto1.Packs (bus-policy gated) --
    void uninstallPack(const QString& rpmName);

signals:
    // Bridged from each daemon's OperationCompleted (demux on op).
    void iconsApplied();
    void iconsRestored();
    void densityEnabled();
    void packUninstalled(const QString& rpmName);

    // Bridged from the launcher daemon's Progress broadcast so the GUI can
    // show a real completion ratio instead of an indeterminate spinner.
    void iconProgress(const QString& op, int done, int total);

    // op is the daemon-side method name ("ApplyIcons", "UninstallPack",
    // ...). Fired on daemon error, D-Bus transport failure, or when the
    // icon-ops lock stays busy past the client wait budget. Lets QML
    // clear settings.isRunning.
    void error(const QString& op, const QString& message);

private slots:
    void onThemesOperationCompleted(const QString& op, bool ok,
                                    const QString& message);
    void onPacksOperationCompleted(const QString& op, bool ok,
                                   const QString& message);
    void onLauncherProgress(const QString& op, int done, int total);

    // Poll for the session daemon after startLauncherDaemonDetached().
    void onLauncherWaitTick();
    // Poll until icon-ops.lock is free (boot update-icons / prior apply).
    void onLockWaitTick();

private:
    // Wrap a fire-and-forget D-Bus method call. On transport failure
    // emit error(op, ...) so the GUI's busy state drains.
    void asyncCall(const QString& op, const QVariantList& args);

    // Subscribe to the per-interface broadcast signals once, before any
    // method is invoked.
    void hookBroadcastSignals();

    // Common entry for ApplyIcons / RestoreIcons: wait briefly if the
    // icon-ops lock is taken, dispatch when the daemon is on the bus,
    // otherwise kick it and retry from onLauncherWaitTick().
    void queueIconOp(const QString& op, const QVariantList& args);
    void dispatchPendingIconOp();
    void failPendingIconOp(const QString& message);
    void startLockWait(const QString& op, const QVariantList& args);

    // Private: callers go through HelperClient::instance() or the
    // QML `Helper` singleton.
    HelperClient();
    Q_DISABLE_COPY(HelperClient)

    QTimer*      _launcherWait;
    QTimer*      _lockWait;
    int          _launcherWaitedMs;
    int          _lockWaitedMs;
    QString      _pendingIconOp;
    QVariantList _pendingIconArgs;
    bool         _hooked;
};

#endif // HELPERCLIENT_H
