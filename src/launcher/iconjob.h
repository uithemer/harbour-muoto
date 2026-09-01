#ifndef ICONJOB_H
#define ICONJOB_H

#include <QString>
#include <QStringList>

// One unit of icon work. Everything that used to call into LauncherIconOps
// directly -- D-Bus, dconf watches, the desktop-directory watcher, apkd
// container readiness, the dynamic-icon tick -- now describes what it wants and
// hands it to IconJobQueue instead of running it wherever it happened to fire.
struct IconJob
{
    enum Kind
    {
        ApplyAll = 0,     // full apply of a pack
        Restore,          // full restore to stock
        RefreshDesktops,  // newly installed/updated entries only
        RefreshApk,       // apkd bridge entries only, after a container restart
        Rebuild,          // re-attach updaters (dconf changed)
        RebuildDyn        // a dynamic icon wants to redraw
    };

    Kind kind = Rebuild;
    QString pack;
    bool runPack = false;
    bool overlay = false;
    QStringList paths;      // RefreshDesktops
    bool scheduleVerify = false; // RefreshApk
    quint64 id = 0;

    // What the D-Bus adaptor reports for this job, empty for internal work.
    QString dbusOp;
};

#endif // ICONJOB_H
