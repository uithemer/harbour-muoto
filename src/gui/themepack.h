#ifndef THEMEPACK_H
#define THEMEPACK_H

#include <QObject>

// ThemePack: small QML-facing facade for the few non-privileged
// helpers the GUI still needs. Read-only environment introspection
// plus the user-bus `systemctl --user restart lipstick.service` shim
// used by the homescreen-restart dialog.
class ThemePack : public QObject
{
    Q_OBJECT

    public:
        explicit ThemePack(QObject* parent = 0);

    public slots:
        bool hasStoremanInstalled() const;
        void restartHomescreen();
};

#endif // THEMEPACK_H
