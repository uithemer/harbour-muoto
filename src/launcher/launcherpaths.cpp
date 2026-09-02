#include "launcherpaths.h"
#include "iconpaths.h"

#include <QCryptographicHash>
#include <QStandardPaths>

namespace LauncherPaths {

QString muotoShare()
{
    return IconPaths::muotoShare();
}

QString generatedIconsDir()
{
    return muotoShare() + QStringLiteral("/launcher-icons");
}

QString manifestPath()
{
    return muotoShare() + QStringLiteral("/launcher-manifest.json");
}

QString dynamicIconsDir()
{
    return muotoShare() + QStringLiteral("/dynamic-icons");
}

QString dynamicIconsPluginDir()
{
    return dynamicIconsDir();
}

QString perAppProviderKey(const QString& desktopBaseName)
{
    return QStringLiteral("/apps/harbour-muoto/launcher/applications/%1/provider")
        .arg(desktopBaseName);
}

QString savedIconKey(const QString& desktopBaseName)
{
    return QStringLiteral("/apps/harbour-muoto/launcher/saved-id/%1")
        .arg(desktopBaseName);
}

QString fingerprintKey(const QString& normalizedPath)
{
    return QStringLiteral("/apps/harbour-muoto/launcher/fingerprint/%1")
        .arg(normalizedPath);
}

QString desktopInodeKey(const QString& desktopBaseName)
{
    return QStringLiteral("/apps/harbour-muoto/launcher/desktop-inode/%1").arg(desktopBaseName);
}

QString iconBackupPath(const QString& iconPath)
{
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString key = QCryptographicHash::hash(iconPath.toLatin1(), QCryptographicHash::Sha1).toHex();
    return QStringLiteral("%1/harbour-muoto/launcher-backup/%2").arg(dataPath, key);
}

QString desktopBackupPath(const QString& desktopPath)
{
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString key = QCryptographicHash::hash(desktopPath.toLatin1(),
                                                 QCryptographicHash::Sha1).toHex();
    return QStringLiteral("%1/harbour-muoto/desktop-backup/%2").arg(dataPath, key);
}

bool isOurGeneratedIconPath(const QString& iconPath)
{
    return iconPath.startsWith(generatedIconsDir());
}

} // namespace LauncherPaths
