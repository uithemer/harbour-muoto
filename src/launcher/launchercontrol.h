#ifndef LAUNCHERCONTROL_H
#define LAUNCHERCONTROL_H

// Restart or stop the user-session launcher icon daemon.
void restartLauncherIconDaemon();
void stopLauncherIconDaemon();

// Restore .desktop redirects and generated PNGs (manifest-driven).
bool restoreLauncherManifest();

#endif // LAUNCHERCONTROL_H
