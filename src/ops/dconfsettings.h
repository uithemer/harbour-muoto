#ifndef DCONFSETTINGS_H
#define DCONFSETTINGS_H

// Per-user settings for Muoto (QML ConfigurationGroup uses path).
// From C++, read/write/reset any key under path via runDconfAsDefaultUser()
// in dconfuser.h only — never as root's user dconf DB.

namespace DconfSettings
{
    constexpr const char *path = "/apps/harbour-muoto";
    constexpr const char *homeRefreshKey = "/apps/harbour-muoto/homeRefresh";
}

#endif
