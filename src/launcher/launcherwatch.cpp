#include "launcherwatch.h"

#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTimer>

namespace {

const char* kRearmSuffix = ".muoto-rearm";

// Lipstick's LauncherMonitor holds directory events back for 2000 ms before it
// acts on them, and cancels a pending remove against a later add of the same
// name. Renaming an entry away and back inside that window therefore leaves the
// launcher item and its grid position untouched, while QFileSystemWatcher still
// re-adds the file watch for the path.
const int kSettleMs = 400;
const int kHoldbackMs = 2400;

QString asidePath(const QString& desktopPath)
{
    return desktopPath + QLatin1String(kRearmSuffix);
}

void waitMs(int ms)
{
    // A nested loop rather than a blocking sleep: the daemon has to keep
    // answering D-Bus while we sit out Lipstick's holdback.
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

} // namespace

void LauncherWatch::rearmDesktopWatches(const QStringList& desktopPaths)
{
    if(desktopPaths.isEmpty())
        return;

    QStringList moved;
    moved.reserve(desktopPaths.size());

    for(const QString& desktopPath : desktopPaths)
    {
        const QString aside = asidePath(desktopPath);
        QFile::remove(aside);
        if(QFile::rename(desktopPath, aside))
            moved.append(desktopPath);
        else
            qWarning() << "muoto-launcher: could not re-arm watch for" << desktopPath;
    }

    if(moved.isEmpty())
        return;

    // Two separate directory scans are needed. Without the gap Lipstick can
    // fold both renames into one scan, see an unchanged file list and never
    // re-add the watch.
    waitMs(kSettleMs);

    for(const QString& desktopPath : moved)
    {
        if(!QFile::rename(asidePath(desktopPath), desktopPath))
            qWarning() << "muoto-launcher: could not restore" << desktopPath << "after re-arm";
    }

    // Let the holdback expire so the watch is live before callers rewrite Icon=.
    waitMs(kHoldbackMs);

    qInfo() << "muoto-launcher: re-armed launcher watches for" << moved.size() << "desktop entries";
}

void LauncherWatch::waitForMonitorHoldback()
{
    waitMs(kHoldbackMs);
}

void LauncherWatch::sweepStaleRearmFiles(const QStringList& directories)
{
    const QString suffix = QLatin1String(kRearmSuffix);

    for(const QString& dirPath : directories)
    {
        QDir dir(dirPath);
        if(!dir.exists())
            continue;

        const QStringList leftovers = dir.entryList({QLatin1Char('*') + suffix}, QDir::Files);
        for(const QString& name : leftovers)
        {
            const QString aside = dir.absoluteFilePath(name);
            QString desktopPath = aside;
            desktopPath.chop(suffix.size());

            // The entry is already back; the copy aside is just debris.
            if(QFile::exists(desktopPath))
            {
                QFile::remove(aside);
                continue;
            }

            if(QFile::rename(aside, desktopPath))
                qWarning() << "muoto-launcher: recovered" << desktopPath << "after interrupted re-arm";
            else
                qWarning() << "muoto-launcher: could not recover" << desktopPath;
        }
    }
}
