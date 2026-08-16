#ifndef ICONUPDATER_P_H
#define ICONUPDATER_P_H

#include <QString>

class IconProvider;

class IconUpdaterPrivate
{
public:
    IconUpdaterPrivate(IconProvider* provider, const QString& desktopPath,
                       IconUpdater::Mode mode);

    void updateMonitoredIcon();
    void restoreMonitoredIcon();
    void updateNonMonitoredIcon();
    void restoreNonMonitoredIcon();

    IconProvider* provider;
    QString desktopPath;
    QString iconPath;
    bool monitoredIcon;
    bool alienDalvikIcon;
    bool forceRedirect;
};

#endif // ICONUPDATER_P_H
