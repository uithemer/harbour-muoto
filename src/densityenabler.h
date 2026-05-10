#ifndef DENSITYENABLER_H
#define DENSITYENABLER_H

#include <QObject>
#include <QString>

// DensityEnabler: idempotent C++ port of the legacy tps/enable-dpi.sh.
//
// Sailfish ships system-wide dconf locks under
//   /etc/dconf/db/vendor.d/locks/silica-configs.txt
//   /etc/dconf/db/vendor.d/locks/ui-configs.txt
// that pin Silica/UI keys (e.g. icon_size_launcher, theme_pixel_ratio)
// to their vendor defaults. UI Themer needs to mutate those keys for the
// "Display density" page to take effect, so the lock files have to be
// moved out of the way. ensureEnabled() does that once: it relocates the
// lock files into /usr/share/sailfishos-uithemer/backup/dlocks/<name>.bk,
// runs `dconf update`, then snapshots defaultuser's current
// /desktop/sailfish/silica/icon_size_launcher into uithemer's own dconf
// path (/desktop/lipstick/sailfishos-uithemer/iconSizeLauncherSeed) so we
// have a non-vendor-locked baseline to fall back to.
//
// Idempotent: safe to call repeatedly. If the .bk files already exist
// (i.e. ensureEnabled() ran before, e.g. from %post on package install),
// the per-file move is skipped. The seed dconf key is only written when
// missing, so user-edited values are never clobbered.
//
// Serialises on the same FileLock sentinel as IconApplier / FontApplier
// so density / icon / font jobs never race each other.
//
// Aliendalvik build.prop reading (the legacy script's other side effect)
// is intentionally not ported: Android-DPI plumbing has been dropped from
// the C++/QML surface in 2.5.4.
class DensityEnabler : public QObject
{
    Q_OBJECT

public:
    explicit DensityEnabler(QObject* parent = nullptr);

public slots:
    // Move vendor dconf locks into the uithemer backup dir, refresh the
    // dconf db, and seed iconSizeLauncherSeed. No-op on subsequent calls.
    // Always emits enabled() (success or logged failure) so QML callers'
    // one-shot connections always drain.
    void ensureEnabled();

signals:
    void enabled();
    void error(const QString& message);

private:
    bool moveLockToBackup(const QString& fileName);
    void runDconfUpdate();
    QString readDefaultUserDconf(const QString& key);
    void writeDefaultUserDconf(const QString& key, const QString& value);

    static const char* kVendorLocksDir;
    static const char* kBackupDir;
    static const char* kIconSizeLauncherKey;
    static const char* kIconSizeSeedKey;
};

#endif // DENSITYENABLER_H
