#include "launcherapplymode.h"
#include "dconfuser.h"

bool launcherInstantApplyEnabled()
{
    QString out;
    if(!runDconfAsDefaultUser({QStringLiteral("read"),
                               QStringLiteral("/apps/harbour-muoto/launcherInstantApply")},
                              &out))
        return true;

    out.remove(QLatin1Char('\''));
    if(out.isEmpty())
        return true;

    return out == QLatin1String("true");
}
