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

    // Retries for up to timeoutMs before giving up. The sentinel is shared with
    // font and density work, and launcher-icond now holds it across a whole
    // queue drain rather than a single operation, so failing fast on a held lock
    // would reject far more user-initiated work than it used to. Blocks, so only
    // call it off the GUI thread or with a short timeout.
    static bool waitFor(FileLock* lock, int timeoutMs,
                        const QString& path = defaultLockPath());

private:
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;

    // Retry the non-blocking acquire on an instance that did not get the lock.
    void reacquire();

    int _fd;
    QString _path;
};

#endif // FILELOCK_H
