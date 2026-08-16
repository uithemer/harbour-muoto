#ifndef ICONUPDATER_P_H
#define ICONUPDATER_P_H

#include <QString>

class IconProvider;

class IconUpdaterPrivate
{
public:
    IconUpdaterPrivate(IconProvider* provider, const QString& desktopPath,
                       IconUpdater::Mode mode);

    bool updateMonitoredIcon();
    void restoreMonitoredIcon();
    bool updateNonMonitoredIcon();
    void restoreNonMonitoredIcon();

    IconProvider* provider;
    QString desktopPath;
    QString iconPath;
    bool monitoredIcon;
    bool alienDalvikIcon;
    bool forceRedirect;
    bool lastUpdateOk = false;
};

#endif // ICONUPDATER_P_H
