#ifndef LAUNCHERPATHS_H
#define LAUNCHERPATHS_H

#include "muotolauncherglobal.h"
#include <QString>

namespace LauncherPaths {

MUOTO_LAUNCHER_EXPORT QString muotoShare();
MUOTO_LAUNCHER_EXPORT QString generatedIconsDir();
MUOTO_LAUNCHER_EXPORT QString manifestPath();
MUOTO_LAUNCHER_EXPORT QString dynamicIconsDir();
MUOTO_LAUNCHER_EXPORT QString dynamicIconsPluginDir();

MUOTO_LAUNCHER_EXPORT QString perAppProviderKey(const QString& desktopBaseName);
MUOTO_LAUNCHER_EXPORT QString savedIconKey(const QString& desktopBaseName);
MUOTO_LAUNCHER_EXPORT QString fingerprintKey(const QString& normalizedPath);
MUOTO_LAUNCHER_EXPORT QString iconBackupPath(const QString& iconPath);

MUOTO_LAUNCHER_EXPORT bool isOurGeneratedIconPath(const QString& iconPath);

} // namespace LauncherPaths

#endif // LAUNCHERPATHS_H
