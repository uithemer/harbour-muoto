#ifndef DCONFSETTINGS_H
#define DCONFSETTINGS_H

// Per-user settings for UI Themer (QML ConfigurationGroup uses path).
// From C++, read/write/reset any key under path via runDconfAsDefaultUser()
// in dconfuser.h only — never as root's user dconf DB.

namespace DconfSettings
{
    constexpr const char *path = "/apps/sailfishos-uithemer";
    constexpr const char *homeRefreshKey = "/apps/sailfishos-uithemer/homeRefresh";
}

#endif
