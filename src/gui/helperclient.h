#ifndef HELPERCLIENT_H
#define HELPERCLIENT_H

#include <QObject>
#include <QString>

class QDBusInterface;
class QDBusMessage;
class QQmlEngine;
class QJSEngine;

// HelperClient: process-wide Q_OBJECT facade around the
// org.uithemer.UiThemer1 system-bus service. Exposed to QML as the
// `Helper` singleton (registered via qmlRegisterSingletonType in
// main()), and shared with C++ peers (ThemePackModel, ThemePack) via
// HelperClient::instance(). Singleton because each instance opens its
// own match rule on the system bus, and we used to create up to six
// of them (one per QML element instantiation) — wasteful and noisy.
//
// All slots return immediately. Real success / failure / progress
// comes through the matching Qt signal; if the daemon is not
// available, the matching error() signal fires synchronously so the
// GUI's busy spinner clears.
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
    // -- Themes interface (manage-themes) --
    void applyIcons(const QString& pack, bool overlay);
    void restoreIcons();
    void reassertIcons();
    void refreshOriginals();
    // ThemeNewDesktops is the unified rescan: drift reassert +
    // uninstall cleanup + new-theming in one daemon-side pass. The
    // GUI's watcher passes settings.iconOverlay so newly-installed
    // apps that don't match the pack still get an overlay when the
    // user opted in at apply time.
    void themeNewDesktops(bool overlay);
    void densityEnable();

    // -- Packs interface (manage-packs) --
    void uninstallPack(const QString& rpmName);

signals:
    // Bridged 1:1 from the daemon's per-interface OperationCompleted
    // broadcast (via demux on the op string). Each signal mirrors
    // exactly one of the Qt signals the existing IconApplier /
    // DensityEnabler / ThemePack / ThemePackModel surface emitted
    // when the work was still done in-process.
    void iconsApplied();
    void iconsRestored();
    void iconsReasserted();
    void originalsRefreshed();
    void newDesktopsThemed(int count);
    void densityEnabled();
    void packUninstalled(const QString& rpmName);

    void progress(int done, int total);

    // op is the daemon-side method name ("ApplyIcons", "UninstallPack",
    // ...). Fired on polkit deny, daemon error, or D-Bus transport
    // failure. The GUI typically just logs; the matching positive-
    // result signal still does NOT fire in this case, but the
    // daemon's OperationCompleted(false,...) broadcast triggers this
    // error and lets QML clear `settings.isRunning`.
    void error(const QString& op, const QString& message);

private slots:
    void onThemesOperationCompleted(const QString& op, bool ok,
                                    const QString& message);
    void onThemesProgress(const QString& op, int done, int total);
    void onPacksOperationCompleted(const QString& op, bool ok,
                                   const QString& message);

private:
    // Lazy-construct a per-interface QDBusInterface on the system bus.
    QDBusInterface* themesIface();
    QDBusInterface* packsIface();

    // Wrap a fire-and-forget D-Bus method call. On transport failure
    // emit error(op, ...) so the GUI's busy state drains.
    void asyncCall(QDBusInterface* iface, const QString& op,
                   const QVariantList& args);

    // Subscribe to the per-interface OperationCompleted (and Themes
    // Progress) broadcast signals once, before any method is invoked.
    void hookBroadcastSignals();

    // Private: callers go through HelperClient::instance() or the
    // QML `Helper` singleton.
    HelperClient();
    Q_DISABLE_COPY(HelperClient)

    QDBusInterface* _themes;
    QDBusInterface* _packs;
    bool            _hooked;
};

#endif // HELPERCLIENT_H
