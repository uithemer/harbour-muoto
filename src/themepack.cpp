#include "themepack.h"
#include "spawner.h"
#include <unistd.h>
#include <QFileInfo>
#include <QDebug>
#include <QProcess>

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
    // Avoid attempting to escalate privileges from the UI.
    // If you need to know the privileged user, implement this in the helper.
    QProcess proc;
    proc.start("whoami");
    proc.waitForFinished(1000);
    return QString(proc.readAllStandardOutput()).trimmed();
}

void ThemePack::restartHomescreen() const
{
    // Removed setuid_ex(0). Execution of privileged scripts must be handled by a helper/service.
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/homescreen.sh", [this]() { emit homescreenRestarted(); });
}

void ThemePack::installDependencies() const
{
    // Removed setuid_ex(0). Use a privileged helper for package installation.
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/install_dependencies.sh", [this]() { emit dependenciesInstalled(); });
}

void ThemePack::installImageMagick() const
{
    // Removed setuid_ex(0). Use a privileged helper for package installation.
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/install_imagemagick.sh", [this]() { emit imageMagickInstalled(); });
}

void ThemePack::enableddensity() const
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-dpi.sh", [this]() { emit serviceChanged(); });
}

void ThemePack::disableddensity() const
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-dpi.sh", [this]() { emit serviceChanged(); });
}

void ThemePack::restoreIZ() const
{
    // Removed setuid_ex(0). Restoration requiring root must be done by the privileged helper.
    Spawner::executeSync("/usr/share/sailfishos-uithemer/scripts/restore_iz.sh");
}

void ThemePack::enableserviceautoupdate() const
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-autoupdate.sh", [this]() { });
}

void ThemePack::disableserviceautoupdate() const
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-autoupdate.sh", [this]() { });
}

void ThemePack::enableservicesu() const
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-servicesu.sh", [this]() { emit serviceChanged(); });
}

void ThemePack::disableservicesu() const
{
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-servicesu.sh", [this]() { emit serviceChanged(); });
}

QString ThemePack::getTimer() const
{
    return Spawner::executeSync("cat /usr/share/harbour-themepacksupport/service/hours");
}

void ThemePack::applyHours(const QString& hours) const
{
    Spawner::executeSync("/usr/share/sailfishos-uithemer/scripts/apply_hours.sh " + hours);
}

void ThemePack::hideIcon() const
{
    // Removed setuid_ex(0). Modifying system application files requires root — move to helper.
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