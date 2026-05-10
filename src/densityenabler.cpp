#include "densityenabler.h"
#include "filelock.h"
#include "spawner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QDebug>

#include <unistd.h>

const char* DensityEnabler::kVendorLocksDir =
    "/etc/dconf/db/vendor.d/locks";
const char* DensityEnabler::kBackupDir =
    "/usr/share/sailfishos-uithemer/backup/dlocks";
const char* DensityEnabler::kIconSizeLauncherKey =
    "/desktop/sailfish/silica/icon_size_launcher";
const char* DensityEnabler::kIconSizeSeedKey =
    "/desktop/lipstick/sailfishos-uithemer/iconSizeLauncherSeed";

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

QString DensityEnabler::readDefaultUserDconf(const QString& key)
{
    QProcess p;
    p.start(QStringLiteral("su"),
            QStringList()
                << QStringLiteral("-")
                << QStringLiteral("defaultuser")
                << QStringLiteral("-c")
                << (QStringLiteral("dconf read ") + key));
    p.waitForStarted();
    p.waitForFinished(15000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

void DensityEnabler::writeDefaultUserDconf(const QString& key,
                                          const QString& value)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(QStringLiteral("su"),
            QStringList()
                << QStringLiteral("-")
                << QStringLiteral("defaultuser")
                << QStringLiteral("-c")
                << (QStringLiteral("dconf write ") + key
                    + QLatin1Char(' ') + value));
    p.waitForStarted();
    p.waitForFinished(15000);
}

void DensityEnabler::ensureEnabled()
{
    FileLock lk;
    if(!lk.isHeld())
    {
        emit error(QStringLiteral("could not acquire lock"));
        emit enabled();
        return;
    }

    if(setuid(0) != 0)
    {
        // Not fatal: we may already be root. Log only when we are not.
        if(geteuid() != 0)
            qWarning() << "DensityEnabler: setuid(0) failed and not euid 0";
    }

    QDir().mkpath(QString::fromLatin1(kBackupDir));

    bool ok = true;
    ok &= moveLockToBackup(QStringLiteral("silica-configs.txt"));
    ok &= moveLockToBackup(QStringLiteral("ui-configs.txt"));
    if(!ok)
    {
        emit error(QStringLiteral("failed to relocate one or more vendor locks"));
        // continue: still refresh dconf and try to seed
    }

    runDconfUpdate();

    const QString existingSeed = readDefaultUserDconf(
        QString::fromLatin1(kIconSizeSeedKey));
    if(existingSeed.isEmpty())
    {
        const QString launcherValue = readDefaultUserDconf(
            QString::fromLatin1(kIconSizeLauncherKey));
        if(!launcherValue.isEmpty())
        {
            writeDefaultUserDconf(
                QString::fromLatin1(kIconSizeSeedKey), launcherValue);
        }
    }

    emit enabled();
}
