#ifndef DENSITYENABLER_H
#define DENSITYENABLER_H

#include <QObject>
#include <QString>
#include <QStringList>

// DensityEnabler: idempotent C++ port of the legacy tps/enable-dpi.sh
// plus the C++ replacement for the retired restore_dpi.sh / restore_dpr.sh
// / restore_iz.sh trio.
//
// Sailfish ships system-wide dconf locks under
//   /etc/dconf/db/vendor.d/locks/silica-configs.txt
//   /etc/dconf/db/vendor.d/locks/ui-configs.txt
// that pin Silica/UI keys (e.g. icon_size_launcher, theme_pixel_ratio)
// to their vendor defaults. Muoto needs to mutate those keys for the
// "Display density" page to take effect, so the lock files have to be
// moved out of the way. ensureEnabled() does that once: it relocates the
// lock files into /usr/share/harbour-muoto/backup/dlocks/<name>.bk,
// then runs `dconf update`.
//
// Idempotent: safe to call repeatedly. If the .bk files already exist
// (i.e. ensureEnabled() ran before, e.g. from %post on package install),
// the per-file move is skipped.
//
// restoreDensity(dpr, iconSize) dconf-resets
// /desktop/sailfish/silica/{theme_pixel_ratio,icon_size_launcher} via
// runDconfAsDefaultUser() (never root's user DB). Always emits restored().
//
// Uses the same non-blocking FileLock sentinel as icon / font ops
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
    // Move vendor dconf locks into the muoto backup dir and refresh the
    // dconf db. No-op on subsequent calls. Always emits enabled() (success
    // or logged failure) so QML callers' one-shot connections always drain.
    void ensureEnabled();

    void restoreDensity(bool dpr, bool iconSize);

signals:
    void enabled();
    void restored();
    void error(const QString& message);

private:
    bool moveLockToBackup(const QString& fileName);
    // System-wide compile after relocating vendor lock files (root only).
    void runDconfUpdate();
    void runUserDconf(const QStringList& args);

    static const char* kVendorLocksDir;
    static const char* kBackupDir;
    static const char* kThemePixelRatioKey;
    static const char* kIconSizeLauncherKey;
};

#endif // DENSITYENABLER_H
