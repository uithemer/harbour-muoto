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

void mgconfSetBool(const char* path, bool value)
{
    MGConfItem item(QString::fromLatin1(path));
    item.set(value);
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

void LauncherSettings::setIconOverlay(bool enabled)
{
    mgconfSetBool("/apps/harbour-muoto/iconOverlay", enabled);
}

bool LauncherSettings::dynamicClockEnabled()
{
    return mgconfBool("/apps/harbour-muoto/launcher/dynamicClockEnabled", true);
}

bool LauncherSettings::dynamicCalendarEnabled()
{
    return mgconfBool("/apps/harbour-muoto/launcher/dynamicCalendarEnabled", true);
}

void LauncherSettings::setDynamicClockEnabled(bool enabled)
{
    mgconfSetBool("/apps/harbour-muoto/launcher/dynamicClockEnabled", enabled);
}

void LauncherSettings::setDynamicCalendarEnabled(bool enabled)
{
    mgconfSetBool("/apps/harbour-muoto/launcher/dynamicCalendarEnabled", enabled);
}
