#ifndef ICONAPPLIER_H
#define ICONAPPLIER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

class QFileSystemWatcher;
class IconManifest;

class IconApplier : public QObject
{
    Q_OBJECT

public:
    explicit IconApplier(QObject* parent = 0);

public slots:
    // Quick capability counts used by the UI to decide whether to expose icon options.
    int nativeMatchCount(const QString& packName) const;
    int apkMatchCount(const QString& packName) const;

    // Apply theme to all matching .desktop files. If overlay is true, missing icons
    // get a generated overlay PNG dropped into the per-user cache and Icon= is pointed
    // at it.
    void applyIcons(const QString& packName, bool overlay);

    // Restore originals for every entry currently in the manifest.
    void restoreIcons();

    // Re-apply themed_icon for every manifest entry whose .desktop still exists.
    // Self-healing:
    //   - if the current Icon= differs from both themed_icon and original_icon
    //     (typical after a package update that rewrote Icon=), record cur as
    //     the new original_icon BEFORE rewriting (avoids the "lost original"
    //     race when Refresh originals races with the boot reassert).
    //   - if themed_icon no longer exists on disk (theme pack uninstalled),
    //     write original_icon back and drop the manifest entry instead.
    void reassertCurrentTheme();

    // Re-snapshot original_icon from any .desktop whose current Icon= is not the
    // themed value. Manual escape hatch; reassertCurrentTheme also self-heals.
    void refreshOriginals();

    // Unified rescan triggered by the GUI's QFileSystemWatcher on every
    // change under /usr/share/applications and
    // /home/defaultuser/.local/share/applications. In one FileLock pass:
    //   - drift reassert: any manifest entry whose .desktop's Icon= drifted
    //     (RPM update rewrote it) is snapshotted as the new original then
    //     re-themed; entries pointing at a now-missing themed PNG are rolled
    //     back to original_icon.
    //   - uninstall cleanup: any manifest entry whose .desktop has vanished
    //     is dropped.
    //   - new-theming: any .desktop NOT in the manifest gets themed via the
    //     active pack. If `overlay` is true (mirrors the apply-time choice
    //     the GUI passes in from settings.iconOverlay), an overlay PNG is
    //     composited for entries the pack has no direct icon for.
    // Safe no-op when no theme is active.
    void themeNewDesktops(bool overlay);

    // Enable the QFileSystemWatcher (off by default; the GUI turns it on).
    void enableAutoTheming(bool enable);

    // Build the 3xN montage preview for the confirm page / cover into the
    // process-wide IconPreviewCache. QML reads it via the QQuickImageProvider
    // registered under "uithemer", i.e. image://uithemer/preview/<packName>.
    // Emits previewReady(packName, ok); ok is false when the pack has no
    // PNGs the sampler can find (the dialog flips to a fallback label).
    void buildPreview(const QString& packName);

    // Force lipstick to reload .desktop entries (touches them).
    void touchDesktopFiles() const;

signals:
    void progress(int done, int total);
    void applied();
    void restored();
    void reasserted();
    void originalsRefreshed();
    void previewReady(const QString& packName, bool ok);
    // count is "entries whose Icon= changed in this pass" (newly
    // themed + drift reasserted). Uninstall cleanups are NOT counted
    // -- the .desktop is gone, no lipstick refresh required. QML's
    // top-level Helper.onNewDesktopsThemed handler uses this to
    // decide whether to debounce-restart the homescreen.
    void newDesktopsThemed(int count);
    // Fired from the debounced QFileSystemWatcher slot in the GUI's
    // IconApplier instance. QML hooks this to call
    // Helper.themeNewDesktops(settings.iconOverlay) on the daemon. The
    // local pass would have no privilege to write the system manifest
    // or /usr/share/applications/*.desktop, so we don't even try here.
    void watcherFired();

private slots:
    void onWatchedDirChanged(const QString& path);
    void debouncedRescan();

private:
    QString manifestPath() const;
    QString packDir(const QString& packName) const;

    // Pack lookup helpers. Each returns the absolute path to the matching PNG inside
    // the pack, or QString() if the pack has no icon for that base.
    // Native lookup keys off the .desktop's Icon= value (not its filename), so a
    // jolla-camera.desktop with Icon=icon-launcher-camera matches
    // <pack>/.../icon-launcher-camera.png. Searches <pack>/native/<size>/apps/
    // first, then falls back to the pre-3.0 <pack>/jolla/<zSize>/icons/ subtree.
    QString findNativeIcon(const QString& packName, const QString& iconValue) const;
    QString findApkIcon(const QString& packName, const QString& base) const;

    // Generate (or reuse from cache) an overlay-composited PNG and return its path.
    QString makeOverlayIcon(const QString& packName, const QString& base,
                            const QString& kind, const QString& sourceIcon) const;

    // Resolve a .desktop's current Icon= value to an actual PNG path, when possible
    // (used as the inner image when generating overlays). May return QString().
    QString resolveSourceIcon(const QString& iconValue, const QString& kind) const;

    // List native + APK .desktop files we should consider.
    QStringList nativeDesktops() const;
    QStringList apkDesktops() const;

    // Compute the "base" name used for matching against the pack:
    // - native: file basename without extension (e.g. /usr/share/applications/foo.desktop -> "foo").
    // - apk: the value of Icon= itself (e.g. "apkd_launcher_org.example.bar").
    QString baseForNative(const QString& desktopPath) const;
    QString baseForApk(const QString& iconValue) const;

    QString cacheOverlayDir() const;

    void chownToDefaultUser(const QString& path) const;

    // Drift reassert + uninstall cleanup pass. Caller must hold the
    // FileLock and own the `mf` IconManifest; this helper does not
    // load or save. Returns the number of entries it rewrote
    // (`reasserted`) and the number it dropped (`removed`) so callers
    // can decide whether to touchDesktopFiles().
    void reassertWithinLock(IconManifest& mf,
                            int& reasserted, int& removed);

    QFileSystemWatcher* _watcher;
    QTimer _watchDebounce;
};

#endif // ICONAPPLIER_H
