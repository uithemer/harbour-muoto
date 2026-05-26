#include "themepack.h"

#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QDebug>

ThemePack::ThemePack(QObject* parent)
    : QObject(parent)
{
}

bool ThemePack::hasAndroidSupport() const
{
    return QFileInfo("/vendor/build.prop").exists()
        || QFileInfo("/opt/alien/system/build.prop").exists();
}

bool ThemePack::hasStoremanInstalled() const
{
    return QFileInfo("/usr/share/harbour-storeman/qml/harbour-storeman.qml").exists();
}

qint64 ThemePack::getFileSize(const QString& file)
{
    QFileInfo fi("/usr/share/" + file);
    return fi.size();
}

QString ThemePack::whoami() const
{
    // 2.6.0: no setuid escalation. Whatever uid the GUI runs under is
    // what we return. Used only by AboutPage as a sanity
    // check; defaultuser is the expected value.
    QProcess p;
    p.start(QStringLiteral("whoami"));
    p.waitForFinished(2000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

void ThemePack::restartHomescreen()
{
    // 2.6.0: scripts/homescreen.sh is retired. The GUI runs as
    // defaultuser, so the user-bus connection systemctl --user needs
    // to talk to defaultuser's user systemd already exists in this
    // process. startDetached and forget; we emit homescreenRestarted
    // optimistically (the lipstick respawn typically completes in
    // <1s and the QML side mostly cares about clearing isRunning).
    QProcess::startDetached(QStringLiteral("systemctl"),
        QStringList() << QStringLiteral("--user")
                      << QStringLiteral("restart")
                      << QStringLiteral("lipstick.service"));
    emit homescreenRestarted();
}
