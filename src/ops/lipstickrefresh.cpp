#include "lipstickrefresh.h"
#include "iconpaths.h"
#include "iconpackrunner.h"
#include "iconoverlay.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>

#include <sys/stat.h>

namespace
{
    const char* kNativeAppsDir = "/usr/share/applications";

    QString stripTrailingEpochHash(QString line)
    {
        if(!line.startsWith(QLatin1String("Icon=")))
            return line;

        int end = line.length();
        if(end > 0 && line.at(end - 1) == QLatin1Char('\n'))
            --end;

        const int hash = line.lastIndexOf(QLatin1Char('#'), end - 1);
        if(hash < 0 || hash <= 4) // no #, or # inside "Icon="
            return line;

        int i = hash + 1;
        while(i < end && line.at(i).isDigit())
            ++i;

        if(i <= hash + 1 || i != end)
            return line;

        return line.left(hash) + line.mid(end);
    }

    void stampApkLauncherIconTimestamps()
    {
        const qint64 epoch = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000;
        QDir apk(IconPaths::liveApkApplicationsDir());
        if(!apk.exists())
            return;

        int updated = 0;
        const QStringList names = apk.entryList(
            QStringList() << QStringLiteral("apkd_launcher_*.desktop"), QDir::Files);

        for(const QString& name : names)
        {
            const QString path = apk.absoluteFilePath(name);
            QFile file(path);
            if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            QStringList lines;
            bool changed = false;
            while(!file.atEnd())
            {
                QString line = QString::fromUtf8(file.readLine());
                if(line.startsWith(QLatin1String("Icon=")))
                {
                    line = stripTrailingEpochHash(line);
                    if(line.endsWith(QLatin1Char('\n')))
                        line.chop(1);
                    line += QStringLiteral("#%1\n").arg(epoch);
                    changed = true;
                }
                lines << line;
            }
            file.close();

            if(!changed)
                continue;
            if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
                continue;

            for(const QString& line : lines)
                file.write(line.toUtf8());

            ++updated;
        }

        if(updated > 0)
            qInfo() << "muoto: stamped Icon= epoch on" << updated << "apkd launcher desktops";
    }
}

void unstampApkLauncherIconTimestamps()
{
    QDir apk(IconPaths::liveApkApplicationsDir());
    if(!apk.exists())
        return;

    int updated = 0;
    const QStringList names = apk.entryList(
        QStringList() << QStringLiteral("apkd_launcher_*.desktop"), QDir::Files);

    for(const QString& name : names)
    {
        const QString path = apk.absoluteFilePath(name);
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QStringList lines;
        bool changed = false;
        while(!file.atEnd())
        {
            QString line = QString::fromUtf8(file.readLine());
            if(line.startsWith(QLatin1String("Icon=")))
            {
                const QString cleaned = stripTrailingEpochHash(line);
                if(cleaned != line)
                {
                    line = cleaned;
                    changed = true;
                }
            }
            lines << line;
        }
        file.close();

        if(!changed)
            continue;
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            continue;

        for(const QString& line : lines)
            file.write(line.toUtf8());

        ++updated;
    }

    if(updated > 0)
        qInfo() << "muoto: stripped Icon= epoch from" << updated << "apkd launcher desktops";
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
    stampApkLauncherIconTimestamps();

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
