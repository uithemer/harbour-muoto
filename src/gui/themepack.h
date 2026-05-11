#ifndef THEMEPACK_H
#define THEMEPACK_H

#include <QObject>

// ThemePack: small QML-facing facade for the few non-privileged
// helpers the GUI still needs. 2.7.0 trimmed every SystemServices-
// related slot (autoupdate timer, servicesu, hideIcon, applyHours,
// getTimer) along with the OptionsPage that used them; what remains
// is read-only environment introspection plus the user-bus
// `systemctl --user restart lipstick.service` shim used by the
// homescreen-restart dialog.
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
        qint64 getFileSize(const QString& file);
        void restartHomescreen();

    signals:
        void homescreenRestarted();
};

#endif // THEMEPACK_H
