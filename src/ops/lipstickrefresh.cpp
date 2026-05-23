#include "lipstickrefresh.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>

#include <pwd.h>
#include <sys/stat.h>

namespace
{
    const char* kNativeAppsDir = "/usr/share/applications";
    const char* kApkAppsDir = "/home/defaultuser/.local/share/applications";
}

namespace
{
    const char* kHomeRefreshKey =
        "/desktop/lipstick/sailfishos-uithemer/homeRefresh";
}

bool homeRefreshEnabledInDconf()
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(QStringLiteral("su"),
            QStringList()
                << QStringLiteral("-")
                << QStringLiteral("defaultuser")
                << QStringLiteral("-c")
                << (QStringLiteral("dconf read ") + QLatin1String(kHomeRefreshKey)));
    if(!p.waitForFinished(5000))
    {
        p.kill();
        return false;
    }
    if(p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0)
        return false;

    const QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    if(out.isEmpty())
        return false;
    return out == QLatin1String("true");
}

bool touchPathForLauncher(const QString& path)
{
    if(path.isEmpty())
        return false;

    QFile file(path);
    if(!file.exists() || !file.open(QIODevice::Append))
        return false;

    return futimens(file.handle(), nullptr) == 0;
}

bool restartDefaultUserLipstick()
{
    if(!homeRefreshEnabledInDconf())
        return false;

    struct passwd* pw = getpwnam("defaultuser");
    if(!pw)
        return false;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("XDG_RUNTIME_DIR"),
               QStringLiteral("/run/user/") + QString::number(
                   static_cast<qulonglong>(pw->pw_uid)));
    env.insert(QStringLiteral("HOME"), QString::fromUtf8(pw->pw_dir));
    env.insert(QStringLiteral("USER"), QStringLiteral("defaultuser"));
    env.insert(QStringLiteral("LOGNAME"), QStringLiteral("defaultuser"));

    QProcess p;
    p.setProcessEnvironment(env);
    return p.startDetached(QStringLiteral("systemctl"),
                           QStringList()
                               << QStringLiteral("--user")
                               << QStringLiteral("restart")
                               << QStringLiteral("lipstick.service"));
}

void touchAllLauncherDesktops()
{
    QDir native(QString::fromLatin1(kNativeAppsDir));
    const QStringList nativeList = native.entryList(
        QStringList() << QStringLiteral("*.desktop"), QDir::Files);
    for(const QString& n : nativeList)
        touchPathForLauncher(native.absoluteFilePath(n));

    QDir apk(QString::fromLatin1(kApkAppsDir));
    if(!apk.exists())
        return;

    const QStringList apkList = apk.entryList(
        QStringList() << QStringLiteral("apkd_launcher_*.desktop"), QDir::Files);
    for(const QString& n : apkList)
        touchPathForLauncher(apk.absoluteFilePath(n));
}

void notifyLauncherAfterIconOp()
{
    touchAllLauncherDesktops();
    restartDefaultUserLipstick();
}
