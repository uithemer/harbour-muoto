#include "launchercontrol.h"
#include "launchermanifest.h"

#include <QProcess>

namespace {

bool runSystemctlUser(const QStringList& args)
{
    QProcess p;
    p.start(QStringLiteral("systemctl"), QStringList() << QStringLiteral("--user") << args);
    if(!p.waitForFinished(10000))
    {
        p.kill();
        return false;
    }
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

} // namespace

void restartLauncherIconDaemon()
{
    runSystemctlUser({QStringLiteral("try-restart"),
                      QStringLiteral("harbour-muoto-launcher-icond.service")});
}

void stopLauncherIconDaemon()
{
    runSystemctlUser({QStringLiteral("stop"),
                      QStringLiteral("harbour-muoto-launcher-icond.service")});
}

bool restoreLauncherManifest()
{
    return LauncherManifest::restoreAll();
}
