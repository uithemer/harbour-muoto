#ifndef FILELOCK_H
#define FILELOCK_H

#include <QString>

// RAII wrapper around POSIX flock(2) on a sentinel file.
// Used to serialise concurrent UI Themer operations across processes:
//   - helperd applyIcons/restoreIcons/reassertCurrentTheme/refreshOriginals
//   - helperd themeNewDesktops auto-theming watcher
//
// All paths use a single lock sentinel sibling to the manifest:
//   /usr/share/sailfishos-uithemer/icon-backup.lock
//
// Usage:
//   {
//       FileLock lk;                      // acquires LOCK_EX (blocking)
//       // ... read manifest, mutate desktops, write manifest ...
//   }                                     // releases on scope exit
class FileLock
{
public:
    // Default sentinel path. Override only for tests.
    explicit FileLock(const QString& path = defaultLockPath(), bool blocking = true);
    ~FileLock();

    bool isHeld() const { return _fd >= 0; }

    static QString defaultLockPath();

private:
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;

    int _fd;
    QString _path;
};

#endif // FILELOCK_H
