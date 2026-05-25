#include "lipstickrefresh.h"
#include "dconfsettings.h"
#include "iconpaths.h"
#include "iconpackrunner.h"
#include "iconoverlay.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDebug>

#include <pwd.h>
#include <sys/stat.h>

namespace
{
    const char* kNativeAppsDir = "/usr/share/applications";

    const QString kIconKey = QStringLiteral("Icon=");

    bool redirectApkIconLine(QString& line, const QString& packName, bool* usedPackPath)
    {
        if(usedPackPath)
            *usedPackPath = false;

        if(!line.startsWith(kIconKey))
            return false;

        const QString launcherSeg = IconPaths::apkLauncherIconSegment();
        if(!line.contains(launcherSeg))
            return false;

        const QString basename = line.mid(kIconKey.size()).section(QLatin1Char('/'), -1);
        if(basename.isEmpty())
            return false;

        const QString customPath = IconPaths::liveApkCustomDir() + basename;
        if(QFileInfo::exists(customPath))
        {
            line.replace(launcherSeg, IconPaths::apkCustomSegment());
            return true;
        }

        const QString packPath = IconPaths::packApkPngPath(packName, basename);
        if(packPath.isEmpty())
            return false;

        line = kIconKey + packPath;
        if(usedPackPath)
            *usedPackPath = true;
        return true;
    }

    bool revertApkIconLine(QString& line)
    {
        if(!line.startsWith(kIconKey))
            return false;

        if(line.contains(IconPaths::apkLauncherIconSegment()))
            return false;

        const QString basename = line.mid(kIconKey.size()).section(QLatin1Char('/'), -1);
        if(basename.isEmpty())
            return false;

        line = kIconKey + IconPaths::liveApkLauncherDir() + basename;
        return true;
    }

    bool writeApkDesktopRevert(const QString& desktopPath)
    {
        QFile in(desktopPath);
        if(!in.open(QIODevice::ReadOnly))
            return false;

        const QString content = QString::fromUtf8(in.readAll());
        in.close();

        QStringList lines = content.split(QLatin1Char('\n'));
        bool changed = false;

        for(int i = 0; i < lines.size(); ++i)
        {
            QString line = lines.at(i);
            if(!revertApkIconLine(line))
                continue;

            lines[i] = line;
            changed = true;
            break;
        }

        if(!changed)
            return false;

        QFile out(desktopPath);
        if(!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;

        const bool hadTrailingNl = content.endsWith(QLatin1Char('\n'));
        QByteArray outData = lines.join(QLatin1Char('\n')).toUtf8();
        if(hadTrailingNl && !outData.isEmpty() && !outData.endsWith('\n'))
            outData.append('\n');

        if(out.write(outData) != outData.size())
        {
            out.close();
            return false;
        }
        out.close();

        IconPaths::chownToDefaultUser(desktopPath);
        touchPathForLauncher(desktopPath);
        return true;
    }

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
                << (QStringLiteral("dconf read ")
                    + QLatin1String(DconfSettings::homeRefreshKey)));
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

    QDir apk(IconPaths::liveApkApplicationsDir());
    if(!apk.exists())
        return;

    const QStringList apkList = apk.entryList(
        QStringList() << QStringLiteral("apkd_launcher_*.desktop"), QDir::Files);
    for(const QString& n : apkList)
        touchPathForLauncher(apk.absoluteFilePath(n));
}

void redirectApkDesktopsToCustom(const QString& packName)
{
    QDir apk(IconPaths::liveApkApplicationsDir());
    if(!apk.exists())
        return;

    int updatedCustom = 0;
    int updatedPack = 0;
    int skipped = 0;

    const QStringList apkList = apk.entryList(
        QStringList() << QStringLiteral("apkd_launcher_*.desktop"), QDir::Files);
    for(const QString& n : apkList)
    {
        const QString path = apk.absoluteFilePath(n);
        QFile in(path);
        if(!in.open(QIODevice::ReadOnly))
        {
            ++skipped;
            continue;
        }

        const QString content = QString::fromUtf8(in.readAll());
        in.close();

        QStringList lines = content.split(QLatin1Char('\n'));
        bool changed = false;
        bool usedPack = false;

        for(int i = 0; i < lines.size(); ++i)
        {
            QString line = lines.at(i);
            if(!redirectApkIconLine(line, packName, &usedPack))
                continue;

            lines[i] = line;
            changed = true;
            break;
        }

        if(!changed)
        {
            ++skipped;
            continue;
        }

        QFile out(path);
        if(!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            ++skipped;
            continue;
        }

        const bool hadTrailingNl = content.endsWith(QLatin1Char('\n'));
        QByteArray outData = lines.join(QLatin1Char('\n')).toUtf8();
        if(hadTrailingNl && !outData.isEmpty() && !outData.endsWith('\n'))
            outData.append('\n');

        if(out.write(outData) != outData.size())
        {
            out.close();
            ++skipped;
            continue;
        }
        out.close();

        IconPaths::chownToDefaultUser(path);
        touchPathForLauncher(path);

        if(usedPack)
            ++updatedPack;
        else
            ++updatedCustom;
    }

    qInfo() << "uithemer: APK desktop redirect custom" << updatedCustom << "pack" << updatedPack
            << "skipped" << skipped;
}

void revertApkDesktopsToLauncherIcon()
{
    QDir apk(IconPaths::liveApkApplicationsDir());
    if(!apk.exists())
        return;

    const QStringList apkList = apk.entryList(
        QStringList() << QStringLiteral("apkd_launcher_*.desktop"), QDir::Files);
    for(const QString& n : apkList)
        writeApkDesktopRevert(apk.absoluteFilePath(n));
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

    redirectApkDesktopsToCustom(packName);
    IconPaths::chownApkLauncherTree();

    return any;
}

void removeApkCustomDir()
{
    QDir custom(IconPaths::liveApkCustomDir());
    if(custom.exists())
        custom.removeRecursively();
}

void notifyLauncherAfterIconOp(bool apkIconsTouched)
{
    Q_UNUSED(apkIconsTouched);
    touchAllLauncherDesktops();
}
