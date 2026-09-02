#ifndef ICONRESOLVE_H
#define ICONRESOLVE_H

#include <QString>
#include <QStringList>

namespace IconResolve {

QString apkBridgeLauncherIconDir();
bool isApkBridgeIcon(const QString& iconPath);
bool isMonitoredIcon(const QString& iconPath);
/** Every existing raster hicolor slot for this icon's basename, including iconPath. */
QStringList hicolorSlotPaths(const QString& iconPath);
/** Pixel size of a hicolor slot path (108 for .../108x108/apps/...), 0 if not one. */
int hicolorSlotSize(const QString& slotPath);
QString resolveIconPath(const QString& iconId);

} // namespace IconResolve

#endif // ICONRESOLVE_H
