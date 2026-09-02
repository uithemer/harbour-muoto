#ifndef ICONUPDATER_P_H
#define ICONUPDATER_P_H

#include <QPointer>
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

    // update() dereferences this directly rather than going through the signal
    // connection, so a freed provider would be a crash rather than a no-op.
    QPointer<IconProvider> provider;
    QString desktopPath;
    QString iconPath;
    bool monitoredIcon;
    bool alienDalvikIcon;
    bool forceRedirect;
    bool stablePath = false;
    bool lastUpdateOk = false;
};

#endif // ICONUPDATER_P_H
