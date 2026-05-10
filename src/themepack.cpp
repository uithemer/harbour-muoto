#include "themepack.h"
#include "spawner.h"
#include <unistd.h>
#include <QFileInfo>
#include <QDebug>

ThemePack::ThemePack(QObject* parent): QObject(parent)
{

}

bool ThemePack::hasAndroidSupport() const
{
    bool res = QFileInfo("/vendor/build.prop").exists() || QFileInfo("/opt/alien/system/build.prop").exists();

   qDebug("%d\n", res);
   return res;
}

bool ThemePack::hasStoremanInstalled() const
{
    bool res = QFileInfo("/usr/share/harbour-storeman/qml/harbour-storeman.qml").exists();

   qDebug("%d\n", res);
   return res;
}

qint64 ThemePack::getFileSize(const QString& file)
{
    QFileInfo fi("/usr/share/" + file);
    return fi.size();
}

QString ThemePack::whoami() const
{ 
    setuid_ex(0);
    return Spawner::executeSync("whoami");
}

void ThemePack::restartHomescreen()
{
    // No setuid(0) here: homescreen.sh detects whether it runs as root or as
    // the user and switches to defaultuser via su when needed, so it works
    // regardless of the current uid. Avoiding the elevation keeps the GUI
    // process from becoming permanently root just because the user clicked
    // "restart homescreen" first.
    Spawner::execute("/usr/share/sailfishos-uithemer/homescreen.sh", [this]() mutable { emit homescreenRestarted(); });
}

void ThemePack::enableserviceautoupdate()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/enable-autoupdate.sh", [this]() { });
}

void ThemePack::disableserviceautoupdate()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/disable-autoupdate.sh", [this]() { });
}

void ThemePack::enableservicesu()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/enable-servicesu.sh", [this]() mutable { emit serviceChanged(); });
}

void ThemePack::disableservicesu()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/disable-servicesu.sh", [this]() mutable { emit serviceChanged(); });
}

QString ThemePack::getTimer() const
{
    return Spawner::executeSync("cat /usr/share/sailfishos-uithemer/service/hours");
}

void ThemePack::applyHours(const QString& hours)
{
    Spawner::executeSync("/usr/share/sailfishos-uithemer/apply_hours.sh " + hours);
}

void ThemePack::hideIcon()
{
    setuid_ex(0);
    Spawner::executeSync("echo \"NoDisplay=true\" >> /usr/share/applications/harbour-iconpacksupport.desktop");
}
