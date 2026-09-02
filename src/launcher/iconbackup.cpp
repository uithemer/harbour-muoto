#include "iconbackup.h"
#include "filewrite.h"
#include "launcherpaths.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace IconBackup {

bool exists(const QString& iconPath)
{
    const QString backupPath = LauncherPaths::iconBackupPath(iconPath);
    return QFile::exists(backupPath) && QFileInfo(backupPath).size() > 0;
}

bool create(const QString& iconPath)
{
    if(iconPath.isEmpty() || !QFileInfo::exists(iconPath))
    {
        qWarning() << "muoto-launcher: cannot back up missing icon" << iconPath;
        return false;
    }

    const QString backupPath = LauncherPaths::iconBackupPath(iconPath);
    if(!QFileInfo(backupPath).dir().mkpath(QStringLiteral(".")))
    {
        qWarning() << "muoto-launcher: could not create backup dir for" << iconPath;
        return false;
    }

    QFile::remove(backupPath);
    if(!QFile::copy(iconPath, backupPath) || QFileInfo(backupPath).size() <= 0)
    {
        qWarning() << "muoto-launcher: could not back up" << iconPath;
        QFile::remove(backupPath);
        return false;
    }
    return true;
}

bool restore(const QString& iconPath)
{
    if(iconPath.isEmpty())
        return false;

    const QString backupPath = LauncherPaths::iconBackupPath(iconPath);
    if(!QFile::exists(backupPath) || QFileInfo(backupPath).size() <= 0)
    {
        qWarning() << "muoto-launcher: no usable backup for" << iconPath
                   << "- leaving the live icon alone";
        return false;
    }

    if(!QFileInfo::exists(iconPath))
    {
        // The owning package removed it; recreating it here would only produce a
        // defaultuser-owned orphan that rpm -V flags.
        qInfo() << "muoto-launcher: skipping restore, live icon is gone" << iconPath;
        return false;
    }

    QFile backup(backupPath);
    if(!backup.open(QIODevice::ReadOnly))
    {
        qWarning() << "muoto-launcher: could not read backup for" << iconPath;
        return false;
    }
    const QByteArray content = backup.readAll();
    backup.close();

    if(content.isEmpty())
    {
        qWarning() << "muoto-launcher: empty backup for" << iconPath;
        return false;
    }

    return FileWrite::inPlace(iconPath, content);
}

} // namespace IconBackup
