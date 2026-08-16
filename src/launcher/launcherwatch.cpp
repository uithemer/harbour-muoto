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

enum class Phase
{
    Idle,
    Aside,
    Back,
    Holdback
};

struct RearmState
{
    Phase phase = Phase::Idle;
    QStringList moved;
    std::function<void()> onDone;
    QTimer* timer = nullptr;
};

RearmState& state()
{
    static RearmState s;
    return s;
}

void finish(std::function<void()> onDone)
{
    RearmState& s = state();
    s.phase = Phase::Idle;
    s.moved.clear();
    s.onDone = nullptr;
    if(s.timer)
    {
        s.timer->stop();
        s.timer->deleteLater();
        s.timer = nullptr;
    }
    if(onDone)
        onDone();
}

void restoreMoved(const QStringList& moved)
{
    for(const QString& desktopPath : moved)
    {
        if(!QFile::rename(asidePath(desktopPath), desktopPath))
            qWarning() << "muoto-launcher: could not restore" << desktopPath << "after re-arm";
    }
}

void schedule(int ms, std::function<void()> step)
{
    RearmState& s = state();
    if(!s.timer)
    {
        s.timer = new QTimer;
        s.timer->setSingleShot(true);
    }
    else
    {
        s.timer->disconnect();
        s.timer->stop();
    }
    QObject::connect(s.timer, &QTimer::timeout, s.timer, [step]() { step(); });
    s.timer->start(ms);
}

void moveBackThenHoldback()
{
    RearmState& s = state();
    s.phase = Phase::Back;
    restoreMoved(s.moved);
    const int count = s.moved.size();
    s.moved.clear();

    s.phase = Phase::Holdback;
    schedule(kHoldbackMs, [count]() {
        qInfo() << "muoto-launcher: re-armed launcher watches for" << count << "desktop entries";
        auto done = state().onDone;
        finish(done);
    });
}

} // namespace

void LauncherWatch::rearmDesktopWatches(const QStringList& desktopPaths,
                                        std::function<void()> onDone)
{
    if(desktopPaths.isEmpty())
    {
        if(onDone)
            onDone();
        return;
    }

    RearmState& s = state();
    if(s.phase != Phase::Idle)
    {
        qWarning() << "muoto-launcher: re-arm requested while another is in progress";
        if(onDone)
            onDone();
        return;
    }

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
    {
        if(onDone)
            onDone();
        return;
    }

    // Two separate directory scans are needed. Without the gap Lipstick can
    // fold both renames into one scan, see an unchanged file list and never
    // re-add the watch.
    s.phase = Phase::Aside;
    s.moved = moved;
    s.onDone = std::move(onDone);
    schedule(kSettleMs, []() { moveBackThenHoldback(); });
}

void LauncherWatch::waitForMonitorHoldback(std::function<void()> onDone)
{
    RearmState& s = state();
    if(s.phase != Phase::Idle)
    {
        qWarning() << "muoto-launcher: holdback wait while re-arm in progress";
        if(onDone)
            onDone();
        return;
    }

    s.phase = Phase::Holdback;
    s.onDone = std::move(onDone);
    schedule(kHoldbackMs, []() {
        auto done = state().onDone;
        finish(done);
    });
}

bool LauncherWatch::rearmInProgress()
{
    return state().phase != Phase::Idle;
}

void LauncherWatch::abortAndRecover(const QStringList& directories)
{
    RearmState& s = state();
    if(s.timer)
    {
        s.timer->stop();
        s.timer->disconnect();
        s.timer->deleteLater();
        s.timer = nullptr;
    }

    if(!s.moved.isEmpty())
        restoreMoved(s.moved);

    s.phase = Phase::Idle;
    s.moved.clear();
    s.onDone = nullptr;

    sweepStaleRearmFiles(directories);
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
