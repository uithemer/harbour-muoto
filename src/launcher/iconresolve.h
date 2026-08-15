#ifndef ICONRESOLVE_H
#define ICONRESOLVE_H

#include <QString>

namespace IconResolve {

QString apkBridgeLauncherIconDir();
bool isApkBridgeIcon(const QString& iconPath);
bool isMonitoredIcon(const QString& iconPath);
/** True when the same basename exists under another hicolor size or scalable/. */
bool hasAlternateHicolor(const QString& iconPath);
QString resolveIconPath(const QString& iconId);

} // namespace IconResolve

#endif // ICONRESOLVE_H
