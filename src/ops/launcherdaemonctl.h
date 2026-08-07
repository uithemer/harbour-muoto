#ifndef LAUNCHERDAEMONCTL_H
#define LAUNCHERDAEMONCTL_H

bool ensureLauncherDaemonRunning();
void requestLauncherDaemonRestart();
void requestLauncherDaemonStop();
void requestLauncherManifestRestore();

#endif // LAUNCHERDAEMONCTL_H
