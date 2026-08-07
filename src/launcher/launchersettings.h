#ifndef LAUNCHERSETTINGS_H
#define LAUNCHERSETTINGS_H

#include "muotolauncherglobal.h"
#include <QString>

class MUOTO_LAUNCHER_EXPORT LauncherSettings
{
public:
    static QString activeIconPack();
    static bool iconOverlay();
    static void setIconOverlay(bool enabled);
    static bool dynamicClockEnabled();
    static bool dynamicCalendarEnabled();
    static void setDynamicClockEnabled(bool enabled);
    static void setDynamicCalendarEnabled(bool enabled);
};

#endif // LAUNCHERSETTINGS_H
