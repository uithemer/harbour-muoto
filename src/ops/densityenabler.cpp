#include "densityenabler.h"
#include "dconfuser.h"
#include "filelock.h"
#include "spawner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>

const char* DensityEnabler::kVendorLocksDir =
    "/etc/dconf/db/vendor.d/locks";
const char* DensityEnabler::kBackupDir =
    "/usr/share/harbour-muoto/backup/dlocks";
const char* DensityEnabler::kThemePixelRatioKey =
    "/desktop/sailfish/silica/theme_pixel_ratio";
const char* DensityEnabler::kIconSizeLauncherKey =
    "/desktop/sailfish/silica/icon_size_launcher";

DensityEnabler::DensityEnabler(QObject* parent) : QObject(parent)
{
}

bool DensityEnabler::moveLockToBackup(const QString& fileName)
{
    const QString src = QString::fromLatin1(kVendorLocksDir)
                      + QLatin1Char('/') + fileName;
    const QString dst = QString::fromLatin1(kBackupDir)
                      + QLatin1Char('/') + fileName + QStringLiteral(".bk");

    if(!QFileInfo::exists(src))
        return true;

    if(QFileInfo::exists(dst) && !QFile::remove(dst))
    {
        qWarning() << "DensityEnabler: failed to replace stale backup" << dst;
        return false;
    }

    if(!QFile::rename(src, dst))
    {
        qWarning() << "DensityEnabler: failed to move" << src << "->" << dst;
        return false;
    }
    return true;
}

void DensityEnabler::runDconfUpdate()
{
    // Compiles system dconf after lock-file moves; not a per-user key read/write.
    Spawner::executeSync(QStringLiteral("dconf update"));
}

void DensityEnabler::runUserDconf(const QStringList& args)
{
    if(!runDconfAsDefaultUser(args))
        qWarning() << "DensityEnabler: dconf failed:" << args.join(QLatin1Char(' '));
}

void DensityEnabler::ensureEnabled()
{
    FileLock lk;
    if(!lk.isHeld())
    {
        emit error(QStringLiteral("busy"));
        return;
    }

    // 2.6.0: ensureEnabled() now runs only inside the daemon, which is
    // already root via the dbus-activated unit. The setuid(0) escalation
    // hack is gone. If the GUI ever calls this directly (shouldn't), the
    // QFile::rename calls will fail under defaultuser and emit error()
    // -- the QML callers handle that gracefully.

    QDir().mkpath(QString::fromLatin1(kBackupDir));

    bool ok = true;
    ok &= moveLockToBackup(QStringLiteral("silica-configs.txt"));
    ok &= moveLockToBackup(QStringLiteral("ui-configs.txt"));

    runDconfUpdate();

    if(!ok)
    {
        emit error(QStringLiteral("failed to relocate one or more vendor locks"));
        return;
    }

    emit enabled();
}

void DensityEnabler::restoreDensity(bool dpr, bool iconSize)
{
    FileLock lk;
    if(!lk.isHeld())
    {
        emit error(QStringLiteral("busy"));
        emit restored();
        return;
    }

    if(dpr)
    {
        runUserDconf(QStringList() << QStringLiteral("reset")
                                   << QString::fromLatin1(kThemePixelRatioKey));
    }
    if(iconSize)
    {
        runUserDconf(QStringList() << QStringLiteral("reset")
                                   << QString::fromLatin1(kIconSizeLauncherKey));
    }

    emit restored();
}
