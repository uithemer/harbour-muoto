#include "osupdateguard.h"

#include <QFile>
#include <QProcess>

namespace
{
    const char kOsUpdateFlag[] = "/run/defaultuser/osupdate_running";

    bool systemdUnitActive(const char *unit)
    {
        QProcess proc;
        proc.setProgram(QStringLiteral("systemctl"));
        proc.setArguments({QStringLiteral("is-active"),
                         QStringLiteral("--quiet"),
                         QString::fromLatin1(unit)});
        proc.start(QIODevice::ReadOnly);
        if (!proc.waitForFinished(3000))
            return false;
        return proc.exitCode() == 0;
    }
}

bool OsUpdateGuard::running()
{
    if (QFile::exists(QString::fromLatin1(kOsUpdateFlag)))
        return true;
    if (systemdUnitActive("system-update.target"))
        return true;
    if (systemdUnitActive("sailfish-upgrade-ui.service"))
        return true;
    return false;
}
