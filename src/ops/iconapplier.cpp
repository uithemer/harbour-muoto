#include "iconapplier.h"
#include "desktopfile.h"
#include "iconmanifest.h"
#include "imageutil.h"
#include "iconpreviewcache.h"
#include "filelock.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QImage>
#include <QFileSystemWatcher>
#include <QSet>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>

static const char* kManifestPath = "/usr/share/sailfishos-uithemer/icon-backup.json";
static const char* kNativeAppsDir = "/usr/share/applications";
static const char* kApkAppsDir = "/home/defaultuser/.local/share/applications";
static const char* kPackPrefix = "/usr/share/harbour-themepack-";

IconApplier::IconApplier(QObject* parent)
    : QObject(parent), _watcher(nullptr)
{
    // Debounce: filesystem watchers can fire multiple events per logical
    // change (e.g. RPM doing several link/rename ops). Coalesce them.
    _watchDebounce.setSingleShot(true);
    _watchDebounce.setInterval(750);
    QObject::connect(&_watchDebounce, &QTimer::timeout,
                     this, &IconApplier::debouncedRescan);
}

QString IconApplier::manifestPath() const
{
    return QString::fromLatin1(kManifestPath);
}

QString IconApplier::packDir(const QString& packName) const
{
    // Tolerate either the bare pack name ("numix-circle") or the full RPM /
    // directory name ("harbour-themepack-numix-circle"). MainPage feeds
    // applyIcons() the role-data form (already stripped) while ConfirmPage
    // passes ThemePackModel::packName(int) which keeps the prefix; strip any
    // leading "harbour-themepack-" so we never double up the prefix.
    static const QString kBarePrefix = QStringLiteral("harbour-themepack-");
    QString name = packName;
    if(name.startsWith(kBarePrefix))
        name = name.mid(kBarePrefix.size());

    return QString::fromLatin1(kPackPrefix) + name;
}

QString IconApplier::cacheOverlayDir() const
{
    // Per-user cache for generated overlay PNGs.
    const QString home = QString::fromLatin1("/home/defaultuser");
    return home + QStringLiteral("/.cache/sailfishos-uithemer/overlay");
}

void IconApplier::chownToDefaultUser(const QString& path) const
{
    struct passwd* pw = getpwnam("defaultuser");
    if(!pw)
        return;
    if(chown(path.toLocal8Bit().constData(), pw->pw_uid, pw->pw_gid) != 0)
        qDebug() << "chown failed:" << path;
}

QString IconApplier::findNativeIcon(const QString& packName, const QString& iconValue) const
{
    // Native (post-3.0) layout: <pack>/native/<size>/apps/<key>.png. Largest first
    // so the launcher gets the highest-resolution available asset.
    static const QStringList nativeSizes = {
        QStringLiteral("256x256"),
        QStringLiteral("172x172"),
        QStringLiteral("128x128"),
        QStringLiteral("108x108"),
        QStringLiteral("86x86")
    };
    // Pre-3.0 Sailfish "ambience" layout: <pack>/jolla/<zX.Y>/icons/<key>.png.
    // Order mirrors the original tps/icon-run.sh fallback (largest first).
    static const QStringList jollaSizes = {
        QStringLiteral("z2.0"),
        QStringLiteral("z1.75"),
        QStringLiteral("z1.5-large"),
        QStringLiteral("z1.5"),
        QStringLiteral("z1.25"),
        QStringLiteral("z1.0")
    };

    // Normalise the Icon= value to a bare key so packs can match regardless of
    // how the .desktop spelled it:
    //   - strip leading directory ("/usr/share/icons/.../foo.png" -> "foo")
    //   - strip trailing .png if some package wrote a full filename
    QString key = iconValue;
    if(key.contains(QLatin1Char('/')))
        key = QFileInfo(key).completeBaseName();
    if(key.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
        key.chop(4);
    if(key.isEmpty())
        return QString();

    const QString root = packDir(packName);

    for(const QString& s : nativeSizes)
    {
        const QString p = root + QStringLiteral("/native/") + s
                          + QStringLiteral("/apps/") + key + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }

    for(const QString& s : jollaSizes)
    {
        const QString p = root + QStringLiteral("/jolla/") + s
                          + QStringLiteral("/icons/") + key + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }

    return QString();
}

QString IconApplier::findApkIcon(const QString& packName, const QString& base) const
{
    static const QStringList sizes = {
        QStringLiteral("192x192"),
        QStringLiteral("128x128"),
        QStringLiteral("86x86")
    };

    const QString root = packDir(packName);
    for(const QString& s : sizes)
    {
        const QString p = root + QStringLiteral("/apk/") + s
                          + QStringLiteral("/") + base + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }
    return QString();
}

QStringList IconApplier::nativeDesktops() const
{
    QDir d(kNativeAppsDir);
    QStringList list = d.entryList(QStringList() << QStringLiteral("*.desktop"),
                                   QDir::Files | QDir::Readable);
    QStringList out;
    out.reserve(list.size());
    for(const QString& n : list)
        out << d.absoluteFilePath(n);
    return out;
}

QStringList IconApplier::apkDesktops() const
{
    QDir d(kApkAppsDir);
    if(!d.exists())
        return QStringList();
    QStringList list = d.entryList(QStringList() << QStringLiteral("apkd_launcher_*.desktop"),
                                   QDir::Files | QDir::Readable);
    QStringList out;
    out.reserve(list.size());
    for(const QString& n : list)
        out << d.absoluteFilePath(n);
    return out;
}

QString IconApplier::baseForNative(const QString& desktopPath) const
{
    return QFileInfo(desktopPath).completeBaseName();
}

QString IconApplier::baseForApk(const QString& iconValue) const
{
    // For APK .desktops, the existing convention is `Icon=apkd_launcher_<launcher_id>`
    // (without extension). The pack ships PNGs named exactly that.
    // Strip a directory prefix and `.png` suffix if some package set an absolute path.
    QString v = iconValue;
    if(v.contains(QLatin1Char('/')))
        v = QFileInfo(v).completeBaseName();
    if(v.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
        v.chop(4);
    return v;
}

int IconApplier::nativeMatchCount(const QString& packName) const
{
    if(packName.isEmpty())
        return 0;

    int count = 0;
    const QStringList ds = nativeDesktops();
    for(const QString& d : ds)
    {
        // Native lookup is now keyed off Icon= (so jolla-camera.desktop ->
        // Icon=icon-launcher-camera matches the pack's icon-launcher-camera.png).
        // We have to load the .desktop here to read it.
        DesktopFile df(d);
        if(!df.load())
            continue;
        const QString iv = df.value(QStringLiteral("Icon"));
        if(iv.isEmpty())
            continue;
        if(!findNativeIcon(packName, iv).isEmpty())
            ++count;
    }
    return count;
}

int IconApplier::apkMatchCount(const QString& packName) const
{
    if(packName.isEmpty())
        return 0;

    int count = 0;
    const QStringList ds = apkDesktops();
    for(const QString& dpath : ds)
    {
        DesktopFile df(dpath);
        if(!df.load())
            continue;
        const QString iv = df.value(QStringLiteral("Icon"));
        if(iv.isEmpty())
            continue;
        const QString base = baseForApk(iv);
        if(!findApkIcon(packName, base).isEmpty())
            ++count;
    }
    return count;
}

QString IconApplier::resolveSourceIcon(const QString& iconValue, const QString& kind) const
{
    if(iconValue.isEmpty())
        return QString();

    // Absolute path: return as-is.
    if(iconValue.startsWith(QLatin1Char('/')) && QFileInfo::exists(iconValue))
        return iconValue;

    if(kind == QStringLiteral("apk"))
    {
        // apkd-bridge keeps the PNG flat under launcherIcon/<icon>.png
        const QString p = QStringLiteral("/home/defaultuser/.local/share/apkd-bridge/launcherIcon/")
                          + iconValue + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }
    else
    {
        // Native: try standard hicolor sizes.
        static const QStringList sizes = {
            QStringLiteral("256x256"),
            QStringLiteral("172x172"),
            QStringLiteral("128x128"),
            QStringLiteral("108x108"),
            QStringLiteral("86x86")
        };
        for(const QString& s : sizes)
        {
            const QString p = QStringLiteral("/usr/share/icons/hicolor/")
                              + s + QStringLiteral("/apps/")
                              + iconValue + QStringLiteral(".png");
            if(QFileInfo::exists(p))
                return p;
        }
    }
    return QString();
}

QString IconApplier::makeOverlayIcon(const QString& packName, const QString& base,
                                     const QString& kind, const QString& sourceIcon) const
{
    if(sourceIcon.isEmpty())
        return QString();

    const QString overlayBase = ImageUtil::randomOverlayBase(packDir(packName));
    if(overlayBase.isEmpty())
        return QString();

    const QString cacheDir = cacheOverlayDir() + QStringLiteral("/") + packName;
    QDir().mkpath(cacheDir);
    chownToDefaultUser(cacheOverlayDir());
    chownToDefaultUser(cacheDir);

    const QString outPath = cacheDir + QStringLiteral("/") + kind
                            + QStringLiteral("__") + base + QStringLiteral(".png");

    QSize outer, inner;
    if(kind == QStringLiteral("apk"))
    {
        outer = QSize(192, 192);
        inner = QSize(122, 122);
    }
    else
    {
        outer = QSize(172, 172);
        inner = QSize(int(172 * 0.6), int(172 * 0.6));
    }

    QImage baseImg(overlayBase);
    QImage innerImg(sourceIcon);
    QImage out = ImageUtil::composite(baseImg, innerImg, outer, inner);
    if(out.isNull())
        return QString();

    if(!out.save(outPath, "PNG"))
        return QString();
    chownToDefaultUser(outPath);
    return outPath;
}

void IconApplier::applyIcons(const QString& packName, bool overlay)
{
    if(packName.isEmpty())
    {
        emit applied();
        return;
    }

    FileLock lk; // serialise vs helper / systemupgrade / autoupdate timer
    Q_UNUSED(lk);

    // Always restore first, so re-applying a different pack starts from clean originals.
    // restoreIcons() also takes the lock; flock is non-recursive across distinct file
    // descriptors but recursive on the SAME fd. Our FileLock opens a new fd each time,
    // so a nested restoreIcons() would deadlock. Inline the restore work here so we
    // hold the lock once across the full apply.
    {
        IconManifest mfRestore(manifestPath());
        if(mfRestore.load())
        {
            const QHash<QString, IconManifest::Entry> entries = mfRestore.entries();
            for(auto it = entries.begin(); it != entries.end(); ++it)
            {
                const QString dpath = it.key();
                const IconManifest::Entry& e = it.value();

                DesktopFile df(dpath);
                if(!df.exists())
                {
                    mfRestore.removeEntry(dpath);
                    continue;
                }
                if(!df.load())
                    continue;

                df.setValue(QStringLiteral("Icon"), e.originalIcon);
                if(df.save())
                {
                    if(e.kind == QStringLiteral("apk"))
                        chownToDefaultUser(dpath);
                }
                mfRestore.removeEntry(dpath);
            }
            mfRestore.setActiveIconPack(QString());
            mfRestore.save();
        }
    }

    IconManifest manifest(manifestPath());
    manifest.load();
    manifest.setActiveIconPack(packName);

    const QStringList native = nativeDesktops();
    const QStringList apk = apkDesktops();
    const int total = native.size() + apk.size();
    int done = 0;

    auto processOne = [&](const QString& dpath, const QString& kind, bool isApk) {
        ++done;
        emit progress(done, total);

        DesktopFile df(dpath);
        if(!df.load())
            return;

        const QString original = df.value(QStringLiteral("Icon"));
        if(isApk && original.isEmpty())
            return;

        // `base` is still useful as a stable, filesystem-safe key for the overlay
        // cache filename. Native uses the .desktop basename (no slashes, no .png),
        // APK uses the apkd_launcher_<id> from Icon=. The actual pack lookup uses
        // Icon= directly for native (so jolla-camera -> icon-launcher-camera).
        const QString base = isApk ? baseForApk(original) : baseForNative(dpath);

        QString themed = isApk ? findApkIcon(packName, base)
                               : findNativeIcon(packName, original);
        if(themed.isEmpty() && overlay)
            themed = makeOverlayIcon(packName, base, kind,
                                     resolveSourceIcon(original, kind));
        if(themed.isEmpty())
            return;

        // Per-entry commit order matters: write the manifest entry FIRST,
        // then the .desktop file. If we crash between the two, the manifest
        // already records the original_icon so restore still works. If we
        // crash before the manifest write, no harm done — .desktop is
        // unchanged.
        IconManifest::Entry e;
        e.originalIcon = original;
        e.themedIcon = themed;
        e.kind = kind;
        manifest.setEntry(dpath, e);
        manifest.save();

        df.setValue(QStringLiteral("Icon"), themed);
        if(!df.save())
        {
            // Roll back the manifest entry on write failure.
            manifest.removeEntry(dpath);
            manifest.save();
            return;
        }
        if(isApk)
            chownToDefaultUser(dpath);
    };

    for(const QString& dpath : native)
        processOne(dpath, QStringLiteral("native"), false);

    for(const QString& dpath : apk)
        processOne(dpath, QStringLiteral("apk"), true);

    manifest.save();
    touchDesktopFiles();
    emit applied();
}

void IconApplier::restoreIcons()
{
    FileLock lk;
    Q_UNUSED(lk);

    IconManifest manifest(manifestPath());
    if(!manifest.load())
    {
        emit restored();
        return;
    }

    const QHash<QString, IconManifest::Entry> entries = manifest.entries();
    int done = 0;
    const int total = entries.size();

    for(auto it = entries.begin(); it != entries.end(); ++it)
    {
        ++done;
        emit progress(done, total);

        const QString dpath = it.key();
        const IconManifest::Entry& e = it.value();

        DesktopFile df(dpath);
        if(!df.exists())
        {
            // Whole .desktop is gone (app uninstalled). Drop the entry.
            manifest.removeEntry(dpath);
            continue;
        }

        if(!df.load())
            continue;

        df.setValue(QStringLiteral("Icon"), e.originalIcon);
        if(!df.save())
            continue;
        if(e.kind == QStringLiteral("apk"))
            chownToDefaultUser(dpath);

        manifest.removeEntry(dpath);
    }

    manifest.setActiveIconPack(QString());
    manifest.save();
    touchDesktopFiles();
    emit restored();
}

void IconApplier::reassertWithinLock(IconManifest& manifest,
                                     int& reasserted, int& removed)
{
    reasserted = 0;
    removed = 0;

    const QString pack = manifest.activeIconPack();
    if(pack.isEmpty())
        return;

    const QHash<QString, IconManifest::Entry> entries = manifest.entries();

    for(auto it = entries.begin(); it != entries.end(); ++it)
    {
        const QString dpath = it.key();
        IconManifest::Entry e = it.value();

        DesktopFile df(dpath);
        if(!df.exists())
        {
            // Uninstall cleanup: .desktop is gone (app uninstalled).
            manifest.removeEntry(dpath);
            ++removed;
            continue;
        }
        if(!df.load())
            continue;

        const QString cur = df.value(QStringLiteral("Icon"));

        // (a) Self-heal: if Icon= drifted to something that's neither the
        // themed value nor the recorded original, the upstream package
        // changed it. Snapshot it as the new original BEFORE we clobber.
        if(!cur.isEmpty() && cur != e.themedIcon && cur != e.originalIcon)
        {
            e.originalIcon = cur;
            manifest.setEntry(dpath, e);
        }

        // (b) Theme pack uninstalled / themed PNG vanished: write the
        // original back instead of pointing at a missing path.
        if(!QFileInfo::exists(e.themedIcon))
        {
            df.setValue(QStringLiteral("Icon"), e.originalIcon);
            if(df.save())
            {
                if(e.kind == QStringLiteral("apk"))
                    chownToDefaultUser(dpath);
                manifest.removeEntry(dpath);
                ++removed;
            }
            continue;
        }

        // (c) Normal case: re-write Icon= to the themed value.
        if(cur == e.themedIcon)
            continue;

        df.setValue(QStringLiteral("Icon"), e.themedIcon);
        if(df.save())
        {
            if(e.kind == QStringLiteral("apk"))
                chownToDefaultUser(dpath);
            ++reasserted;
        }
    }

    // If every themed_icon vanished, the active pack is effectively gone.
    if(manifest.entries().isEmpty())
        manifest.setActiveIconPack(QString());
}

void IconApplier::reassertCurrentTheme()
{
    FileLock lk;
    Q_UNUSED(lk);

    IconManifest manifest(manifestPath());
    if(!manifest.load())
    {
        emit reasserted();
        return;
    }

    int reassertedCount = 0;
    int removedCount = 0;
    reassertWithinLock(manifest, reassertedCount, removedCount);

    if(reassertedCount > 0 || removedCount > 0)
    {
        manifest.save();
        touchDesktopFiles();
    }
    emit reasserted();
}

void IconApplier::refreshOriginals()
{
    FileLock lk;
    Q_UNUSED(lk);

    IconManifest manifest(manifestPath());
    if(!manifest.load())
    {
        emit originalsRefreshed();
        return;
    }

    bool changed = false;
    QHash<QString, IconManifest::Entry> entries = manifest.entries();

    for(auto it = entries.begin(); it != entries.end(); ++it)
    {
        const QString dpath = it.key();
        IconManifest::Entry e = it.value();

        DesktopFile df(dpath);
        if(!df.load())
            continue;

        const QString cur = df.value(QStringLiteral("Icon"));
        // If cur is no longer the themed value, the package update wrote a fresh Icon=:
        // refresh the snapshot of "original" so a future restore returns to that value.
        if(cur != e.themedIcon && !cur.isEmpty())
        {
            e.originalIcon = cur;
            manifest.setEntry(dpath, e);
            changed = true;
        }
    }

    if(changed)
        manifest.save();
    emit originalsRefreshed();
}

void IconApplier::themeNewDesktops(bool overlay)
{
    FileLock lk;
    Q_UNUSED(lk);

    IconManifest manifest(manifestPath());
    if(!manifest.load())
    {
        emit newDesktopsThemed(0);
        return;
    }

    const QString pack = manifest.activeIconPack();
    if(pack.isEmpty())
    {
        emit newDesktopsThemed(0);
        return;
    }

    // First: drift reassert + uninstall cleanup. Fixes entries whose
    // Icon= was rewritten by an RPM update, rolls back entries whose
    // themed PNG vanished, and prunes entries whose .desktop is gone.
    int reassertedCount = 0;
    int removedCount = 0;
    reassertWithinLock(manifest, reassertedCount, removedCount);

    // If the helper pruned the last entry, reassertWithinLock cleared
    // the active pack -- bail before themeing anything new.
    if(manifest.activeIconPack().isEmpty())
    {
        if(reassertedCount > 0 || removedCount > 0)
        {
            manifest.save();
            touchDesktopFiles();
        }
        emit newDesktopsThemed(0);
        return;
    }

    QSet<QString> known;
    const QHash<QString, IconManifest::Entry> entries = manifest.entries();
    for(auto it = entries.begin(); it != entries.end(); ++it)
        known.insert(it.key());

    int themed = 0;

    auto processOne = [&](const QString& dpath, const QString& kind, bool isApk) {
        if(known.contains(dpath))
            return;

        DesktopFile df(dpath);
        if(!df.load())
            return;

        const QString original = df.value(QStringLiteral("Icon"));
        if(isApk && original.isEmpty())
            return;

        // `base` is the filesystem-safe key for the overlay cache.
        // Native uses the .desktop basename; APK uses baseForApk(Icon=).
        // Native pack lookup uses Icon= directly so jolla-* matches.
        const QString base = isApk ? baseForApk(original) : baseForNative(dpath);

        QString themedPath = isApk ? findApkIcon(pack, base)
                                   : findNativeIcon(pack, original);
        // Overlay-on-new: when the user picked the overlay at apply
        // time (passed in via the dconf flag the GUI mirrors), generate
        // a composited icon for any new .desktop the pack has no
        // direct asset for. Mirrors the apply path.
        if(themedPath.isEmpty() && overlay)
            themedPath = makeOverlayIcon(pack, base, kind,
                                         resolveSourceIcon(original, kind));
        if(themedPath.isEmpty())
            return;

        // Same per-entry commit order as applyIcons: manifest first, then file.
        IconManifest::Entry e;
        e.originalIcon = original;
        e.themedIcon = themedPath;
        e.kind = kind;
        manifest.setEntry(dpath, e);
        manifest.save();

        df.setValue(QStringLiteral("Icon"), themedPath);
        if(!df.save())
        {
            manifest.removeEntry(dpath);
            manifest.save();
            return;
        }
        if(isApk)
            chownToDefaultUser(dpath);
        ++themed;
    };

    for(const QString& dpath : nativeDesktops())
        processOne(dpath, QStringLiteral("native"), false);

    for(const QString& dpath : apkDesktops())
        processOne(dpath, QStringLiteral("apk"), true);

    if(themed > 0 || reassertedCount > 0 || removedCount > 0)
    {
        manifest.save();
        touchDesktopFiles();
    }

    emit newDesktopsThemed(themed);
}

void IconApplier::enableAutoTheming(bool enable)
{
    if(enable)
    {
        if(_watcher)
            return;

        _watcher = new QFileSystemWatcher(this);

        QStringList dirs;
        dirs << QStringLiteral("/usr/share/applications");
        dirs << QStringLiteral("/home/defaultuser/.local/share/applications");
        for(const QString& d : dirs)
        {
            if(QFileInfo::exists(d))
                _watcher->addPath(d);
        }

        QObject::connect(_watcher, &QFileSystemWatcher::directoryChanged,
                         this, &IconApplier::onWatchedDirChanged);
    }
    else
    {
        if(!_watcher)
            return;
        _watchDebounce.stop();
        _watcher->deleteLater();
        _watcher = nullptr;
    }
}

void IconApplier::onWatchedDirChanged(const QString& /*path*/)
{
    // Coalesce rapid bursts (RPM transactions, apkd-bridge batch writes).
    _watchDebounce.start();
}

void IconApplier::debouncedRescan()
{
    // The GUI's IconApplier runs as defaultuser and has no privilege
    // to rewrite /usr/share/applications/*.desktop or the system
    // manifest, so calling themeNewDesktops() locally would just emit
    // newDesktopsThemed(0). QML hooks watcherFired and dispatches the
    // call to the daemon via Helper.themeNewDesktops(overlay).
    emit watcherFired();
}

void IconApplier::buildPreview(const QString& packName)
{
    if(packName.isEmpty())
    {
        IconPreviewCache::instance().put(packName, QImage());
        emit previewReady(packName, false);
        return;
    }

    const QStringList sample = ImageUtil::samplePackIcons(packDir(packName), 9);
    QImage img = ImageUtil::montage9(sample);
    const bool ok = !img.isNull();

    IconPreviewCache::instance().put(packName, ok ? img : QImage());
    emit previewReady(packName, ok);
}

void IconApplier::touchDesktopFiles() const
{
    auto touch = [](const QString& dir) {
        QDir d(dir);
        const QStringList files = d.entryList(QStringList() << QStringLiteral("*.desktop"),
                                              QDir::Files);
        for(const QString& f : files)
        {
            const QString p = d.absoluteFilePath(f);
            QFile fd(p);
            if(!fd.open(QIODevice::Append))
                continue;
            fd.close();
            // Bump mtime so lipstick reloads it.
            utimensat(AT_FDCWD, p.toLocal8Bit().constData(), nullptr, 0);
        }
    };
    touch(QString::fromLatin1(kNativeAppsDir));
    touch(QString::fromLatin1(kApkAppsDir));
}
