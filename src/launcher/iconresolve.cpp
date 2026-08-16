#include "iconresolve.h"

#include <silicatheme.h>
#include <silicathemeiconresolver.h>

#include <QCollator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

QStringList findAcceptableSizePaths()
{
    const QString basePath = QStringLiteral("/usr/share/icons/hicolor/");
    QDir dir(basePath);
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(entries.begin(), entries.end(), [&](const QString& s1, const QString& s2) {
        return collator.compare(s1, s2) < 0;
    });

    static QRegularExpression re(QStringLiteral("(\\d+)x\\d+"));
    const int minIconSize = qRound(Silica::Theme::instance()->iconSizeLauncher());
    QStringList paths;

    for(const QString& entry : entries)
    {
        const QString size = re.match(entry).captured(1);
        if(size.isEmpty() || size.toInt() < minIconSize)
            continue;
        paths.append(basePath + entry + QStringLiteral("/apps/"));
    }

    return paths;
}

} // namespace

namespace IconResolve {

QString apkBridgeLauncherIconDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + QStringLiteral("/apkd-bridge/launcherIcon");
}

bool isApkBridgeIcon(const QString& iconPath)
{
    if(iconPath.isEmpty())
        return false;

    const QString bridge = apkBridgeLauncherIconDir();
    if(iconPath.startsWith(bridge + QLatin1Char('/')))
        return true;

    return false;
}

bool isMonitoredIcon(const QString& iconPath)
{
    static QRegularExpression re(QStringLiteral("/usr/share/icons/hicolor/\\w+/apps/.*"));
    return re.match(iconPath).hasMatch();
}

bool hasAlternateHicolor(const QString& iconPath)
{
    if(!isMonitoredIcon(iconPath))
        return false;

    const QFileInfo info(iconPath);
    const QString fileName = info.fileName();
    if(fileName.isEmpty())
        return false;

    const QString baseName = info.completeBaseName();
    const QString hicolorRoot = QStringLiteral("/usr/share/icons/hicolor");
    QDir hicolor(hicolorRoot);
    const QStringList sizeDirs = hicolor.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for(const QString& sizeDir : sizeDirs)
    {
        const QString candidate = hicolorRoot + QLatin1Char('/') + sizeDir
                                  + QStringLiteral("/apps/") + fileName;
        if(candidate != iconPath && QFile::exists(candidate))
            return true;

        // Scalable SVG sibling of a raster (or another scalable path).
        if(sizeDir == QLatin1String("scalable"))
        {
            const QString svg = hicolorRoot + QStringLiteral("/scalable/apps/")
                                + baseName + QStringLiteral(".svg");
            if(svg != iconPath && QFile::exists(svg))
                return true;
        }
    }

    return false;
}

QString resolveIconPath(const QString& iconId)
{
    if(iconId.isEmpty())
        return QString();

    if(iconId.startsWith(QLatin1Char('/')))
    {
        if(isApkBridgeIcon(iconId))
            return iconId;
        if(iconId.startsWith(QStringLiteral("/var/lib/apkd/")))
        {
            const QString bridgePath = apkBridgeLauncherIconDir() + QLatin1Char('/')
                                       + QFileInfo(iconId).fileName();
            if(QFile::exists(bridgePath))
                return bridgePath;
            return QString();
        }
        return iconId;
    }

    const QString bridgePath = apkBridgeLauncherIconDir() + QLatin1Char('/') + iconId;
    if(QFile::exists(bridgePath))
        return bridgePath;

    Silica::ThemeIconResolver iconResolver;
    QString resolvedPath = iconResolver.resolvePath(iconId);

    if(resolvedPath.startsWith(QStringLiteral("/usr/share/icons/hicolor/86x86/apps/")))
    {
        const QString fileName = QFileInfo(resolvedPath).fileName();
        for(const QString& path : findAcceptableSizePaths())
        {
            const QString iconPath = path + fileName;
            if(QFile::exists(iconPath))
                return iconPath;
        }
    }

    return resolvedPath;
}

} // namespace IconResolve
