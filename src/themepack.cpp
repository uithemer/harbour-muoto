#include "themepack.h"
#include "spawner.h"
#include <unistd.h>
#include <QFileInfo>
#include <QDebug>
#include <QProcess>
#include <QDBusInterface>
#include <QDBusReply>

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
    // Prefer asking the privileged helper if available; otherwise fall back to local whoami.
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<QString> r = iface.call("Whoami");
        if(r.isValid()) return r.value();
    }

    QProcess proc;
    proc.start("whoami");
    proc.waitForFinished(1000);
    return QString(proc.readAllStandardOutput()).trimmed();
}

void ThemePack::restartHomescreen() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<bool> r = iface.call("RestartHomescreen");
        if(r.isValid() && r.value()) { emit homescreenRestarted(); return; }
    }

    // Fallback to running the script (non-privileged) - actual restart requires privileged helper.
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/homescreen.sh", [this]() { emit homescreenRestarted(); });
}

void ThemePack::installDependencies() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<bool> r = iface.call("InstallDependencies");
        if(r.isValid() && r.value()) { emit dependenciesInstalled(); return; }
    }

    // Fallback - may fail without privileges
    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/install_dependencies.sh", [this]() { emit dependenciesInstalled(); });
}

void ThemePack::installImageMagick() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<bool> r = iface.call("InstallImageMagick");
        if(r.isValid() && r.value()) { emit imageMagickInstalled(); return; }
    }

    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/install_imagemagick.sh", [this]() { emit imageMagickInstalled(); });
}

void ThemePack::enableddensity() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        iface.call("EnableDensity");
        emit serviceChanged();
        return;
    }

    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-dpi.sh", [this]() { emit serviceChanged(); });
}

void ThemePack::disableddensity() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        iface.call("DisableDensity");
        emit serviceChanged();
        return;
    }

    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-dpi.sh", [this]() { emit serviceChanged(); });
}

void ThemePack::restoreIZ() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<bool> r = iface.call("RestoreIZ");
        if(r.isValid() && r.value()) return;
    }

    Spawner::executeSync("/usr/share/sailfishos-uithemer/scripts/restore_iz.sh");
}

void ThemePack::enableserviceautoupdate() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        iface.call("EnableServiceAutoUpdate");
        return;
    }

    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-autoupdate.sh", [this]() { });
}

void ThemePack::disableserviceautoupdate() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        iface.call("DisableServiceAutoUpdate");
        return;
    }

    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-autoupdate.sh", [this]() { });
}

void ThemePack::enableservicesu() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<bool> r = iface.call("EnableServiceSU");
        if(r.isValid() && r.value()) { emit serviceChanged(); return; }
    }

    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/enable-servicesu.sh", [this]() { emit serviceChanged(); });
}

void ThemePack::disableservicesu() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<bool> r = iface.call("DisableServiceSU");
        if(r.isValid() && r.value()) { emit serviceChanged(); return; }
    }

    Spawner::execute("/usr/share/sailfishos-uithemer/scripts/disable-servicesu.sh", [this]() { emit serviceChanged(); });
}

QString ThemePack::getTimer() const
{
    // Try via helper first
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<QString> r = iface.call("GetTimer");
        if(r.isValid()) return r.value();
    }

    return Spawner::executeSync("cat /usr/share/harbour-themepacksupport/service/hours");
}

void ThemePack::applyHours(const QString& hours) const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        iface.call("ApplyHours", hours);
        return;
    }

    Spawner::executeSync("/usr/share/sailfishos-uithemer/scripts/apply_hours.sh " + hours);
}

void ThemePack::hideIcon() const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        iface.call("HideIcon");
        return;
    }

    Spawner::executeSync("echo \"NoDisplay=true\" >> /usr/share/applications/harbour-iconpacksupport.desktop");
    Spawner::executeSync("echo \"NoDisplay=true\" >> /usr/share/applications/harbour-themepacksupport.desktop");
}

bool ThemePack::getDroidDPI(double *dpi) const
{
    QDBusInterface iface("org.sailfishos.uithemer","/org/sailfishos/uithemer/Helper","org.sailfishos.uithemer", QDBusConnection::systemBus());
    if(iface.isValid()) {
        QDBusReply<double> r = iface.call("GetDroidDPI");
        if(r.isValid()) {
            if(dpi) { *dpi = r.value(); }
            return true;
        }
    }

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