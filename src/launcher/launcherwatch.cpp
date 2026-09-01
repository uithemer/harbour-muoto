#include "launcherwatch.h"

#include <QDebug>
#include <QDir>
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

} // namespace

LauncherRearm::LauncherRearm(QObject* parent)
    : QObject(parent)
{
}

LauncherRearm::~LauncherRearm()
{
    // Anything still aside here would be a launcher entry the user cannot see.
    abortAndRestore();
}

void LauncherRearm::start(const QStringList& desktopPaths)
{
    m_moved.clear();
    m_running = true;

    for(const QString& desktopPath : desktopPaths)
    {
        const QString aside = asidePath(desktopPath);
        QFile::remove(aside);
        if(QFile::rename(desktopPath, aside))
            m_moved.append(desktopPath);
        else
            qWarning() << "muoto-launcher: could not re-arm watch for" << desktopPath;
    }

    if(m_moved.isEmpty())
    {
        done();
        return;
    }

    // Two separate directory scans are needed. Without the gap Lipstick can
    // fold both renames into one scan, see an unchanged file list and never
    // re-add the watch.
    QTimer::singleShot(kSettleMs, this, &LauncherRearm::moveBack);
}

void LauncherRearm::startHoldbackOnly()
{
    m_moved.clear();
    m_running = true;
    emit heartbeat();
    QTimer::singleShot(kHoldbackMs, this, &LauncherRearm::done);
}

void LauncherRearm::moveBack()
{
    if(!m_running)
        return;

    for(const QString& desktopPath : m_moved)
    {
        if(!QFile::rename(asidePath(desktopPath), desktopPath))
            qWarning() << "muoto-launcher: could not restore" << desktopPath << "after re-arm";
    }
    const int count = m_moved.size();
    m_moved.clear();
    emit heartbeat();

    // Let the holdback expire so the watch is live before callers rewrite Icon=.
    QTimer::singleShot(kHoldbackMs, this, [this, count]() {
        qInfo() << "muoto-launcher: re-armed launcher watches for" << count << "desktop entries";
        done();
    });
}

void LauncherRearm::done()
{
    if(!m_running)
        return;
    m_running = false;
    emit finished();
}

void LauncherRearm::abortAndRestore()
{
    m_running = false;
    if(m_moved.isEmpty())
        return;

    qWarning() << "muoto-launcher: restoring" << m_moved.size()
               << "entries left aside by an interrupted re-arm";
    for(const QString& desktopPath : m_moved)
    {
        if(!QFile::rename(asidePath(desktopPath), desktopPath))
            qWarning() << "muoto-launcher: could not restore" << desktopPath;
    }
    m_moved.clear();
}

namespace LauncherWatch {

void sweepStaleRearmFiles(const QStringList& directories)
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

} // namespace LauncherWatch
