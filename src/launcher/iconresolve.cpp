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
    // Raster size slots only. `\w+` also matched `scalable`, so an app whose icon
    // resolved to scalable/apps/foo.svg was treated as an inplace candidate and
    // got PNG bytes written over its SVG.
    static QRegularExpression re(QStringLiteral("/usr/share/icons/hicolor/\\d+x\\d+/apps/.*"));
    return re.match(iconPath).hasMatch();
}

QStringList hicolorSlotPaths(const QString& iconPath)
{
    QStringList slotPaths;
    if(!isMonitoredIcon(iconPath))
        return slotPaths;

    const QString fileName = QFileInfo(iconPath).fileName();
    if(fileName.isEmpty())
        return slotPaths;

    // Raster slots only: scalable/apps SVGs are left alone, PNG bytes must
    // never be written over an SVG.
    static QRegularExpression re(QStringLiteral("^\\d+x\\d+$"));
    const QString hicolorRoot = QStringLiteral("/usr/share/icons/hicolor");
    const QStringList sizeDirs = QDir(hicolorRoot).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(const QString& sizeDir : sizeDirs)
    {
        if(!re.match(sizeDir).hasMatch())
            continue;
        const QString candidate = hicolorRoot + QLatin1Char('/') + sizeDir
                                  + QStringLiteral("/apps/") + fileName;
        if(QFile::exists(candidate))
            slotPaths.append(candidate);
    }
    return slotPaths;
}

int hicolorSlotSize(const QString& slotPath)
{
    static QRegularExpression re(QStringLiteral("/hicolor/(\\d+)x\\d+/apps/"));
    const QRegularExpressionMatch m = re.match(slotPath);
    return m.hasMatch() ? m.captured(1).toInt() : 0;
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
