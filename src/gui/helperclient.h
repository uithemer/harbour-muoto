#ifndef HELPERCLIENT_H
#define HELPERCLIENT_H

#include <QObject>
#include <QString>

class QDBusInterface;
class QDBusMessage;

// HelperClient: Q_OBJECT facade around the org.uithemer.UiThemer1
// system-bus service. The QML side instantiates a single `HelperClient
// { id: helper }` and calls one slot per privileged op; HelperClient
// forwards over D-Bus to the daemon and demuxes the broadcast
// OperationCompleted / Progress signals back into one Qt signal per
// logical event so the existing QML connections (`onThemeApplied`,
// `onDpiRestored`, etc.) keep working with minimal rewiring.
//
// All slots return immediately. Real success / failure / progress
// comes through the matching Qt signal; if the daemon is not
// available, the matching error() signal fires synchronously so the
// GUI's busy spinner clears.
class HelperClient : public QObject
{
    Q_OBJECT

public:
    explicit HelperClient(QObject* parent = nullptr);
    ~HelperClient() override;

public slots:
    // -- Themes interface (manage-themes) --
    void applyIcons(const QString& pack, bool overlay);
    void restoreIcons();
    void reassertIcons();
    void refreshOriginals();
    void themeNewDesktops();
    void densityEnable();

    // -- Packs interface (manage-packs) --
    void uninstallPack(const QString& rpmName);

    // -- SystemServices interface (manage-system-services) --
    void hideIcon();
    void setAutoupdate(bool enabled);
    void setServiceSu(bool enabled);
    void applyHours(const QString& hours);

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
    void serviceChanged();
    void hideIconDone();
    void hoursApplied(const QString& hours);

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
    void onSystemServicesOperationCompleted(const QString& op, bool ok,
                                            const QString& message);

private:
    // Lazy-construct a per-interface QDBusInterface on the system bus.
    QDBusInterface* themesIface();
    QDBusInterface* packsIface();
    QDBusInterface* systemServicesIface();

    // Wrap a fire-and-forget D-Bus method call. On transport failure
    // emit error(op, ...) so the GUI's busy state drains.
    void asyncCall(QDBusInterface* iface, const QString& op,
                   const QVariantList& args);

    // Subscribe to the per-interface OperationCompleted (and Themes
    // Progress) broadcast signals once, before any method is invoked.
    void hookBroadcastSignals();

    QDBusInterface* _themes;
    QDBusInterface* _packs;
    QDBusInterface* _systemServices;
    bool            _hooked;
};

#endif // HELPERCLIENT_H
