#ifndef THEMEPACK_H
#define THEMEPACK_H

#include <QObject>

class HelperClient;

// ThemePack: small QML-facing facade for "stuff the GUI used to do as
// root via Spawner / setuid_ex". 2.6.0 splits its surface in half:
//   - Read-only convenience (whoami, getTimer, getFileSize,
//     hasAndroidSupport, hasStoremanInstalled): unchanged, runs
//     in-process as defaultuser.
//   - restartHomescreen: was setuid + a homescreen.sh shell that
//     ran `systemctl --user restart lipstick.service` against
//     defaultuser's session bus. With the GUI itself now running as
//     defaultuser, this collapses to a single
//     QProcess::startDetached("systemctl", "--user", "restart",
//     "lipstick.service") -- the shell script is gone.
//   - hideIcon, enable/disable autoupdate + servicesu, applyHours:
//     wrap the system bus org.uithemer.UiThemer1.SystemServices
//     methods, gated by the manage-system-services polkit action.
//
// Each privileged slot just forwards to a private HelperClient
// instance; the helper's matching success signal is relayed to
// ThemePack's existing serviceChanged signal so QML callers
// (`onServiceChanged: ...`) keep working without editing the page.
class ThemePack : public QObject
{
    Q_PROPERTY(bool hasAndroidSupport READ hasAndroidSupport CONSTANT FINAL)

    Q_OBJECT

    public:
        explicit ThemePack(QObject* parent = 0);

    public slots:
        bool hasAndroidSupport() const;
        bool hasStoremanInstalled() const;
        QString whoami() const;
        QString getTimer() const;
        qint64 getFileSize(const QString& file);
        void restartHomescreen();
        void applyHours(const QString& hours);
        void enableserviceautoupdate();
        void disableserviceautoupdate();
        void enableservicesu();
        void disableservicesu();
        void hideIcon();

    signals:
        void homescreenRestarted();
        void serviceChanged();

    private:
        HelperClient* _helper;
};

#endif // THEMEPACK_H
