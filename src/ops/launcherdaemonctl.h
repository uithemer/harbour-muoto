#ifndef LAUNCHERDAEMONCTL_H
#define LAUNCHERDAEMONCTL_H

// Session launcher daemon (org.muoto.Launcher1) lifecycle helpers.
//
// Both calls return immediately. Nothing here spins a nested event loop or
// waits on a QProcess: callers run on the GUI's QML thread, where a blocking
// `systemctl --user daemon-reload` costs seconds of frozen UI. Poll
// launcherDaemonRegistered() from a QTimer after kicking the daemon.
bool launcherDaemonRegistered();
void startLauncherDaemonDetached();

#endif // LAUNCHERDAEMONCTL_H
