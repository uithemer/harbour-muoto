#ifndef ICONAPPLIER_H
#define ICONAPPLIER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

class QFileSystemWatcher;

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

    // Theme any .desktop files in the watched dirs that are NOT yet in the
    // manifest, using the active pack from the manifest. Called from the
    // QFileSystemWatcher slot when a new app is installed or apkd-bridge drops
    // a new launcher entry. Safe no-op when no theme is active.
    void themeNewDesktops();

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
    void newDesktopsThemed(int count);

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

    QFileSystemWatcher* _watcher;
    QTimer _watchDebounce;
};

#endif // ICONAPPLIER_H
