#include "launchersettings.h"

#include <MGConfItem>

namespace {

QString mgconfString(const char* path, const QString& defaultValue)
{
    MGConfItem item(QString::fromLatin1(path));
    const QString v = item.value(defaultValue).toString();
    return v;
}

bool mgconfBool(const char* path, bool defaultValue)
{
    MGConfItem item(QString::fromLatin1(path));
    return item.value(defaultValue).toBool();
}

} // namespace

QString LauncherSettings::activeIconPack()
{
    const QString pack = mgconfString("/apps/harbour-muoto/activeIconPack", QStringLiteral("default"));
    if(pack.isEmpty() || pack == QLatin1String("default"))
        return QString();
    return pack;
}

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
