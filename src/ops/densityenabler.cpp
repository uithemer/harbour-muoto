#include "densityenabler.h"
#include "filelock.h"
#include "spawner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QDebug>

const char* DensityEnabler::kVendorLocksDir =
    "/etc/dconf/db/vendor.d/locks";
const char* DensityEnabler::kBackupDir =
    "/usr/share/sailfishos-uithemer/backup/dlocks";
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

    if(QFileInfo::exists(dst))
        return true;
    if(!QFileInfo::exists(src))
        return true;

    if(!QFile::rename(src, dst))
    {
        qWarning() << "DensityEnabler: failed to move" << src << "->" << dst;
        return false;
    }
    return true;
}

void DensityEnabler::runDconfUpdate()
{
    Spawner::executeSync(QStringLiteral("dconf update"));
}

void DensityEnabler::runDefaultUserDconf(const QString& cmd)
{
    // 2.6.0: with the GUI itself running as defaultuser, dconf already
    // reads/writes the right per-user db without any `su -` shell.
    // restoreDensity() is only ever called from the GUI process; the
    // daemon never instantiates DensityEnabler for restore (only for
    // ensureEnabled()), so this code path always runs as defaultuser.
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    QStringList args = cmd.split(QChar(' '), QString::SkipEmptyParts);
    p.start(QStringLiteral("dconf"), args);
    p.waitForStarted();
    p.waitForFinished(15000);
    if(p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0)
    {
        qWarning() << "DensityEnabler: dconf" << cmd
                   << "failed with exit" << p.exitCode();
    }
}

void DensityEnabler::ensureEnabled()
{
    FileLock lk;
    if(!lk.isHeld())
    {
        emit error(QStringLiteral("busy"));
        emit enabled();
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
    if(!ok)
    {
        emit error(QStringLiteral("failed to relocate one or more vendor locks"));
        // continue: still refresh dconf
    }

    runDconfUpdate();

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
        runDefaultUserDconf(QStringLiteral("reset ")
                            + QString::fromLatin1(kThemePixelRatioKey));
    }
    if(iconSize)
    {
        runDefaultUserDconf(QStringLiteral("reset ")
                            + QString::fromLatin1(kIconSizeLauncherKey));
    }

    emit restored();
}
