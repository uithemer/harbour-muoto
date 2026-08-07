#include "themepack.h"

#include <QFileInfo>
#include <QProcess>
#include <QStringList>

ThemePack::ThemePack(QObject* parent)
    : QObject(parent)
{
}

bool ThemePack::hasStoremanInstalled() const
{
    return QFileInfo("/usr/share/harbour-storeman/qml/harbour-storeman.qml").exists();
}

void ThemePack::restartHomescreen()
{
    // GUI runs as defaultuser — user-bus systemctl --user talks to this
    // session's systemd. startDetached and forget.
    QProcess::startDetached(QStringLiteral("systemctl"),
        QStringList() << QStringLiteral("--user")
                      << QStringLiteral("restart")
                      << QStringLiteral("lipstick.service"));
}
