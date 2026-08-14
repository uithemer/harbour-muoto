#ifndef HELPERCLIENT_H
#define HELPERCLIENT_H

#include <QObject>
#include <QString>

class QDBusInterface;
class QDBusMessage;
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
// signal; if the daemon is not available, error() fires synchronously
// so the GUI's busy spinner clears.
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

    // op is the daemon-side method name ("ApplyIcons", "UninstallPack",
    // ...). Fired on daemon error, D-Bus transport failure, or when the
    // theme-op lock is busy. Lets QML clear settings.isRunning.
    void error(const QString& op, const QString& message);

private slots:
    void onThemesOperationCompleted(const QString& op, bool ok,
                                    const QString& message);
    void onPacksOperationCompleted(const QString& op, bool ok,
                                   const QString& message);
    void onNameOwnerChanged(const QString& name, const QString& oldOwner,
                            const QString& newOwner);

private:
    // dbus-activate helperd and drop stale QDBusInterface proxies when the
    // well-known name leaves the bus (helperd idle-quits after 30 s).
    bool ensureHelperService();
    bool ensureLauncherService();
    void dropDBusProxies();

    // Lazy-construct a per-interface QDBusInterface on the system bus.
    QDBusInterface* themesIface();
    QDBusInterface* launcherThemesIface();
    QDBusInterface* packsIface();

    // Wrap a fire-and-forget D-Bus method call. On transport failure
    // emit error(op, ...) so the GUI's busy state drains.
    void asyncCall(const QString& op, const QVariantList& args);

    // Subscribe to the per-interface OperationCompleted broadcast signals
    // once, before any method is invoked.
    void hookBroadcastSignals();

    bool beginIconOpOrError(const QString& op);

    // Private: callers go through HelperClient::instance() or the
    // QML `Helper` singleton.
    HelperClient();
    Q_DISABLE_COPY(HelperClient)

    QDBusInterface* _themes;
    QDBusInterface* _launcherThemes;
    QDBusInterface* _packs;
    bool            _hooked;
};

#endif // HELPERCLIENT_H
