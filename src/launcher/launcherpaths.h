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

// Inode of a .desktop as of our last write. Lipstick loses its per-file watch
// exactly when the inode is replaced (rpm and apkd both install with rename(2)),
// so comparing against this says precisely which entries need the watch put back
// -- and, just as importantly, which do not.
MUOTO_LAUNCHER_EXPORT QString desktopInodeKey(const QString& desktopBaseName);
MUOTO_LAUNCHER_EXPORT QString iconBackupPath(const QString& iconPath);

// Deliberately not under launcher-backup: RestoreIcons wipes that tree once the
// icons are back, and these have to outlive it so a later repair is a local copy
// instead of an rpm download.
MUOTO_LAUNCHER_EXPORT QString desktopBackupPath(const QString& desktopPath);

MUOTO_LAUNCHER_EXPORT bool isOurGeneratedIconPath(const QString& iconPath);

} // namespace LauncherPaths

#endif // LAUNCHERPATHS_H
