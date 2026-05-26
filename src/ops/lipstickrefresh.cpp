#include "lipstickrefresh.h"
#include "iconpaths.h"
#include "iconpackrunner.h"
#include "iconoverlay.h"

#include <QDir>
#include <QFile>

#include <sys/stat.h>

namespace
{
    const char* kNativeAppsDir = "/usr/share/applications";
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

void touchAllLauncherDesktops()
{
    QDir native(QString::fromLatin1(kNativeAppsDir));
    const QStringList nativeList = native.entryList(
        QStringList() << QStringLiteral("*.desktop"), QDir::Files);
    for(const QString& n : nativeList)
        touchPathForLauncher(native.absoluteFilePath(n));

    QDir apk(IconPaths::liveApkApplicationsDir());
    if(!apk.exists())
        return;

    const QStringList apkList = apk.entryList(
        QStringList() << QStringLiteral("apkd_launcher_*.desktop"), QDir::Files);
    for(const QString& n : apkList)
        touchPathForLauncher(apk.absoluteFilePath(n));
}

bool applyApkPhase(const QString& packName, bool runPack, bool overlay, bool* apkIconsTouched)
{
    if(apkIconsTouched)
        *apkIconsTouched = false;

    bool any = false;

    if(runPack)
    {
        IconPackRunner runner;
        bool touched = false;
        if(runner.runApk(packName, &touched))
            any = true;
        if(touched && apkIconsTouched)
            *apkIconsTouched = true;
    }

    if(overlay)
    {
        IconOverlay ov;
        bool touched = false;
        if(ov.applyApk(packName, &touched))
            any = true;
        if(touched && apkIconsTouched)
            *apkIconsTouched = true;
    }

    IconPaths::chownApkLauncherTree();

    return any;
}

void removeApkCustomDir()
{
    QString custom = IconPaths::liveApkLauncherDir();
    const int i = custom.lastIndexOf(QStringLiteral("launcherIcon"));
    if(i < 0)
        return;
    custom.replace(i, QStringLiteral("launcherIcon").size(), QStringLiteral("custom"));
    QDir dir(custom);
    if(dir.exists())
        dir.removeRecursively();
}

void notifyLauncherAfterIconOp(bool apkIconsTouched)
{
    Q_UNUSED(apkIconsTouched);
    touchAllLauncherDesktops();
}
