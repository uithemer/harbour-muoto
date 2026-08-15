#include "launcherdaemonctl.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>

namespace {

const char* kLauncherService = "org.muoto.Launcher1";
const char* kUnitName = "harbour-muoto-launcher-icond.service";
const char* kBinary = "/usr/libexec/harbour-muoto-launcher-icond";
const char* kUnitSource =
    "/usr/lib/systemd/user/harbour-muoto-launcher-icond.service";

QString userSystemdUnitDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QStringLiteral("/systemd/user");
}

// needsReload is set when we just created the link, i.e. systemd has not seen
// the unit name yet.
bool ensureLauncherUnitSymlink(bool* needsReload)
{
    *needsReload = false;

    const QString unitDir = userSystemdUnitDir();
    const QString linkPath = unitDir + QLatin1Char('/') + QLatin1String(kUnitName);
    if(QFile::exists(linkPath))
        return true;

    const QString source = QString::fromLatin1(kUnitSource);
    if(!QFile::exists(source))
    {
        qWarning() << "muoto-launcher: ensureDaemon unit file missing" << source;
        return false;
    }

    QDir().mkpath(unitDir);
    if(QFile::link(source, linkPath))
    {
        qInfo() << "muoto-launcher: ensureDaemon created unit symlink" << linkPath
                << "->" << source;
        *needsReload = true;
        return true;
    }
    if(QFile::exists(linkPath))
        return true;

    qWarning() << "muoto-launcher: ensureDaemon could not create unit symlink";
    return false;
}

} // namespace

bool launcherDaemonRegistered()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if(!bus.isConnected())
        return false;
    QDBusConnectionInterface* iface = bus.interface();
    if(!iface)
        return false;
    const QDBusReply<bool> registered =
        iface->isServiceRegistered(QString::fromLatin1(kLauncherService));
    return registered.isValid() && registered.value();
}

void startLauncherDaemonDetached()
{
    bool needsReload = false;

    if(ensureLauncherUnitSymlink(&needsReload))
    {
        // A single detached shell keeps daemon-reload ordered before start and
        // keeps the binary fallback, without the caller waiting on any of it.
        QString script;
        if(needsReload)
            script += QStringLiteral("systemctl --user daemon-reload; ");
        script += QStringLiteral("systemctl --user start %1 || exec %2")
                      .arg(QLatin1String(kUnitName), QLatin1String(kBinary));

        qInfo() << "muoto-launcher: ensureDaemon starting" << kUnitName
                << "reload=" << needsReload;
        if(QProcess::startDetached(QStringLiteral("/bin/sh"),
                                   QStringList() << QStringLiteral("-c") << script))
            return;
    }

    qInfo() << "muoto-launcher: ensureDaemon starting binary fallback" << kBinary;
    QProcess::startDetached(QString::fromLatin1(kBinary), QStringList());
}
