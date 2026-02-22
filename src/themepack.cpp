#include "themepack.h"
#include "spawner.h"
#include <unistd.h>
#include <QFileInfo>
#include <QDebug>

ThemePack::ThemePack(QObject* parent): QObject(parent)
{

}

QString ThemePack::readDeviceModel() const
{
    QFile file("/usr/share/harbour-themepacksupport/device-model");
    file.open(QFile::ReadOnly);
    QString s = file.readAll().simplified();
    file.close();
    return s;
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

bool ThemePack::hasImageMagickInstalled() const
{
    bool res = QFileInfo("/usr/bin/convert").exists();

   qDebug("%d\n", res);
   return res;
}

double ThemePack::droidDPI() const
{
    double dpi = 0;
    this->getDroidDPI(&dpi);
    return dpi;
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
    setuid_ex(0);
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/homescreen.sh", [this]() mutable { emit homescreenRestarted(); });
}

void ThemePack::installDependencies()
{
    setuid_ex(0);
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/install_dependencies.sh", [this]() mutable { emit dependenciesInstalled(); });
}

void ThemePack::installImageMagick()
{
    setuid_ex(0);
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/install_imagemagick.sh", [this]() mutable { emit imageMagickInstalled(); });
}

void ThemePack::enableddensity()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-dpi.sh", [this]() mutable { emit serviceChanged(); });
}

void ThemePack::disableddensity()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-dpi.sh", [this]() mutable { emit serviceChanged(); });
}

void ThemePack::restoreIZ()
{
    setuid_ex(0);
    Spawner::executeSync("/usr/share/sailfishos-uithemer/scripts/restore_iz.sh");
}

void ThemePack::enableserviceautoupdate()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-autoupdate.sh", [this]() { });
}

void ThemePack::disableserviceautoupdate()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-autoupdate.sh", [this]() { });
}

void ThemePack::enableservicesu()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-servicesu.sh", [this]() mutable { emit serviceChanged(); });
}

void ThemePack::disableservicesu()
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-servicesu.sh", [this]() mutable { emit serviceChanged(); });
}

QString ThemePack::getTimer() const
{
    return Spawner::executeSync("cat /usr/share/harbour-themepacksupport/service/hours");
}

void ThemePack::applyHours(const QString& hours)
{
    Spawner::executeSync("/usr/share/sailfishos-uithemer/scripts/apply_hours.sh " + hours);
}

void ThemePack::hideIcon()
{
    setuid_ex(0);
    Spawner::executeSync("echo \"NoDisplay=true\" >> /usr/share/applications/harbour-iconpacksupport.desktop");
    Spawner::executeSync("echo \"NoDisplay=true\" >> /usr/share/applications/harbour-themepacksupport.desktop");
}

bool ThemePack::getDroidDPI(double *dpi) const
{
    QString s = Spawner::executeSync("cat /usr/share/harbour-themepacksupport/droiddpi-current").simplified();

    if(s.isEmpty())
        return false;

    if(dpi)
    {
        bool ok = false;
        *dpi = s.toDouble(&ok);
        return ok;
    }

    return true;
}
