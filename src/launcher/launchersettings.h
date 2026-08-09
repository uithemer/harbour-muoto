#ifndef LAUNCHERSETTINGS_H
#define LAUNCHERSETTINGS_H

#include "muotolauncherglobal.h"
#include <QString>

class MUOTO_LAUNCHER_EXPORT LauncherSettings
{
public:
    static QString activeIconPack();
    static bool iconOverlay();
    static bool dynamicClockEnabled();
    static bool dynamicCalendarEnabled();
};

#endif // LAUNCHERSETTINGS_H
