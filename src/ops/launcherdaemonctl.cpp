#include "launcherdaemonctl.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

namespace {

const char* kLauncherService = "org.muoto.Launcher1";
const char* kUnitName = "harbour-muoto-launcher-icond.service";
const char* kBinary = "/usr/libexec/harbour-muoto-launcher-icond";

bool launcherServiceRegistered()
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

bool waitForLauncherService(int seconds)
{
    for(int i = 0; i < seconds; ++i)
    {
        if(launcherServiceRegistered())
            return true;
        QEventLoop loop;
        QTimer::singleShot(1000, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return launcherServiceRegistered();
}

QString userSystemdUnitDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QStringLiteral("/systemd/user");
}

bool ensureLauncherUnitSymlink()
{
    const QString unitDir = userSystemdUnitDir();
    const QString linkPath = unitDir + QLatin1Char('/') + QLatin1String(kUnitName);
    if(QFile::exists(linkPath))
        return true;

    static const QStringList unitSources = {
        QStringLiteral("/usr/lib/systemd/user/harbour-muoto-launcher-icond.service"),
        QStringLiteral("/usr/share/harbour-muoto/systemd/user/harbour-muoto-launcher-icond.service"),
    };

    for(const QString& source : unitSources)
    {
        if(!QFile::exists(source))
            continue;
        QDir().mkpath(unitDir);
        if(QFile::link(source, linkPath))
        {
            qInfo() << "muoto-launcher: ensureDaemon created unit symlink" << linkPath
                    << "->" << source;
            return true;
        }
        if(QFile::exists(linkPath))
            return true;
    }
    qWarning() << "muoto-launcher: ensureDaemon could not create unit symlink";
    return false;
}

bool runSystemctlUser(const QStringList& args)
{
    QProcess p;
    p.start(QStringLiteral("systemctl"),
            QStringList() << QStringLiteral("--user") << args);
    if(!p.waitForFinished(15000))
    {
        p.kill();
        return false;
    }
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

void startLauncherBinaryDetached()
{
    qInfo() << "muoto-launcher: ensureDaemon starting binary fallback" << kBinary;
    QProcess::startDetached(QString::fromLatin1(kBinary), QStringList());
}

} // namespace

bool ensureLauncherDaemonRunning()
{
    if(launcherServiceRegistered())
        return true;

    qInfo() << "muoto-launcher: ensureDaemon starting" << kUnitName;
    ensureLauncherUnitSymlink();
    runSystemctlUser({QStringLiteral("daemon-reload")});
    if(runSystemctlUser({QStringLiteral("start"), QString::fromLatin1(kUnitName)}))
    {
        const bool ok = waitForLauncherService(15);
        qInfo() << "muoto-launcher: ensureDaemon systemctl start ok=" << ok;
        return ok;
    }

    startLauncherBinaryDetached();
    const bool ok = waitForLauncherService(15);
    qInfo() << "muoto-launcher: ensureDaemon binary fallback ok=" << ok;
    return ok;
}
