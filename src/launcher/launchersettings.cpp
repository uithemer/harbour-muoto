#include "launchersettings.h"

#include <MGConfItem>

namespace {

bool mgconfBool(const char* path, bool defaultValue)
{
    MGConfItem item(QString::fromLatin1(path));
    return item.value(defaultValue).toBool();
}

} // namespace

bool LauncherSettings::iconOverlay()
{
    return mgconfBool("/apps/harbour-muoto/iconOverlay", false);
}

bool LauncherSettings::dynamicClockEnabled()
{
    return mgconfBool("/apps/harbour-muoto/launcher/dynamicClockEnabled", true);
}

bool LauncherSettings::dynamicCalendarEnabled()
{
    return mgconfBool("/apps/harbour-muoto/launcher/dynamicCalendarEnabled", true);
}
