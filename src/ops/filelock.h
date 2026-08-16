#ifndef FILELOCK_H
#define FILELOCK_H

#include <QString>

// RAII wrapper around POSIX flock(2) on a sentinel file.
// Serialises theme/icon/font/density mutations across GUI, helperd, and
// any D-Bus/systemd caller. Concurrent ops are refused (LOCK_NB), not queued.
//
// Sentinel (co-located with stock icon backup under harbour-muoto/backup/):
//   /usr/share/harbour-muoto/icon-ops.lock
//
// Usage:
//   if (!FileLock::tryProbe()) { ... busy ... }
//   FileLock lk(false);  // hold for an operation; released on scope exit
class FileLock
{
public:
    // Default sentinel path. Override only for tests.
    // blocking=false: flock(LOCK_EX|LOCK_NB); refuses if already held.
    explicit FileLock(const QString& path = defaultLockPath(), bool blocking = false);
    ~FileLock();

    bool isHeld() const { return _fd >= 0; }

    static QString defaultLockPath();

    // Option A GUI probe: acquire NB lock and release before returning.
    static bool tryProbe(const QString& path = defaultLockPath());

private:
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;

    int _fd;
    QString _path;
};

#endif // FILELOCK_H
