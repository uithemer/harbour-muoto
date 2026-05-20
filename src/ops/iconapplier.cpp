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

#include <pwd.h>
#include <unistd.h>

static const char* kManifestPath = "/usr/share/sailfishos-uithemer/icon-backup.json";
static const char* kNativeAppsDir = "/usr/share/applications";
static const char* kApkAppsDir = "/home/defaultuser/.local/share/applications";
static const char* kPackPrefix = "/usr/share/harbour-themepack-";
static const char* kHicolorRoot = "/usr/share/icons/hicolor";

namespace
{
    const QStringList kNativeHicolorSizes = {
        QStringLiteral("256x256"),
        QStringLiteral("172x172"),
        QStringLiteral("128x128"),
        QStringLiteral("108x108"),
        QStringLiteral("86x86"),
    };

    const QStringList kJollaSizes = {
        QStringLiteral("z2.0"),
        QStringLiteral("z1.75"),
        QStringLiteral("z1.5-large"),
        QStringLiteral("z1.5"),
        QStringLiteral("z1.25"),
        QStringLiteral("z1.0"),
    };

    const QStringList kApkPackSizes = {
        QStringLiteral("192x192"),
        QStringLiteral("128x128"),
        QStringLiteral("86x86"),
    };

    QString stripUiThemerSuffix(QString icon)
    {
        static const QString suffixA = QStringLiteral("-uithemer-a");
        static const QString suffixB = QStringLiteral("-uithemer-b");
        if(icon.endsWith(suffixA))
            return icon.left(icon.size() - suffixA.size());
        if(icon.endsWith(suffixB))
            return icon.left(icon.size() - suffixB.size());
        return icon;
    }

    QString iconKeyFromValue(const QString& iconValue)
    {
        QString key = iconValue;
        if(key.contains(QLatin1Char('/')))
            key = QFileInfo(key).completeBaseName();
        if(key.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
            key.chop(4);
        return key;
    }

    bool parseSize(const QString& size, int& w, int& h)
    {
        const int x = size.indexOf(QLatin1Char('x'));
        if(x <= 0)
            return false;
        bool okW = false;
        bool okH = false;
        w = size.left(x).toInt(&okW);
        h = size.mid(x + 1).toInt(&okH);
        return okW && okH && w > 0 && h > 0;
    }
}

IconApplier::IconApplier(QObject* parent)
    : QObject(parent), _watcher(nullptr)
{
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
    return QString::fromLatin1(kPackPrefix) + packShortName(packName);
}

QString IconApplier::packShortName(const QString& packName) const
{
    static const QString kBarePrefix = QStringLiteral("harbour-themepack-");
    QString name = packName;
    if(name.startsWith(kBarePrefix))
        name = name.mid(kBarePrefix.size());
    return name;
}

QString IconApplier::hicolorAppsDir(const QString& packShort, const QString& size) const
{
    return QString::fromLatin1(kHicolorRoot) + QStringLiteral("/") + size
           + QStringLiteral("/apps/") + packShort + QLatin1Char('/');
}

void IconApplier::chownToDefaultUser(const QString& path) const
{
    struct passwd* pw = getpwnam("defaultuser");
    if(!pw)
        return;
    if(chown(path.toLocal8Bit().constData(), pw->pw_uid, pw->pw_gid) != 0)
        qDebug() << "chown failed:" << path;
}

QString IconApplier::nativeAppsSourceDir(const QString& packName, const QString& size) const
{
    const QString apps = packDir(packName) + QStringLiteral("/native/") + size
                         + QStringLiteral("/apps");
    const QFileInfo fi(apps);
    if(!fi.exists())
        return QString();

    if(fi.isSymLink())
    {
        const QString canonical = QFileInfo(fi.canonicalFilePath()).absoluteFilePath();
        return canonical.isEmpty() ? QString() : canonical;
    }

    if(fi.isDir())
        return apps;

    return QString();
}

bool IconApplier::publishPngToAllHicolorSizes(const QString& packShort,
                                             const QString& iconKey,
                                             const QImage& image) const
{
    if(packShort.isEmpty() || iconKey.isEmpty() || image.isNull())
        return false;

    bool any = false;
    for(const QString& size : kNativeHicolorSizes)
    {
        int w = 0;
        int h = 0;
        if(!parseSize(size, w, h))
            continue;

        const QString dir = hicolorAppsDir(packShort, size);
        QDir().mkpath(dir);

        const QString path = dir + iconKey + QStringLiteral(".png");
        const QImage scaled = image.scaled(w, h, Qt::IgnoreAspectRatio,
                                           Qt::SmoothTransformation);
        if(QFile::exists(path))
            QFile::remove(path);
        if(scaled.save(path, "PNG"))
            any = true;
    }

    return any;
}

bool IconApplier::hicolorHasIcon(const QString& packShort, const QString& iconKey) const
{
    for(const QString& size : kNativeHicolorSizes)
    {
        if(QFileInfo::exists(hicolorAppsDir(packShort, size) + iconKey
                             + QStringLiteral(".png")))
            return true;
    }
    return false;
}

bool IconApplier::publishApkKeyToHicolor(const QString& packName, const QString& base) const
{
    const QString shortName = packShortName(packName);
    const QString packPng = findApkIcon(packName, base);
    if(packPng.isEmpty())
        return false;

    bool any = false;
    const QString root = packDir(packName);

    for(const QString& size : kNativeHicolorSizes)
    {
        const QString dir = hicolorAppsDir(shortName, size);
        QDir().mkpath(dir);
        const QString dest = dir + base + QStringLiteral(".png");

        const QString packSized = root + QStringLiteral("/apk/") + size + QLatin1Char('/')
                                  + base + QStringLiteral(".png");
        if(QFileInfo::exists(packSized))
        {
            if(QFile::exists(dest))
                QFile::remove(dest);
            if(QFile::copy(packSized, dest))
                any = true;
            continue;
        }

        QImage master(packPng);
        if(master.isNull())
            continue;

        int w = 0;
        int h = 0;
        if(!parseSize(size, w, h))
            continue;

        const QImage scaled = master.scaled(w, h, Qt::IgnoreAspectRatio,
                                            Qt::SmoothTransformation);
        if(QFile::exists(dest))
            QFile::remove(dest);
        if(scaled.save(dest, "PNG"))
            any = true;
    }

    return any;
}

bool IconApplier::installPackHicolorBridge(const QString& packName)
{
    const QString shortName = packShortName(packName);
    const QString packRoot = packDir(packName);
    if(!QDir(packRoot).exists())
        return false;

    bool ok = false;

    for(const QString& size : kNativeHicolorSizes)
    {
        const QString srcDir = nativeAppsSourceDir(packName, size);
        if(srcDir.isEmpty())
            continue;

        const QString dstDir = hicolorAppsDir(shortName, size);
        QDir().mkpath(dstDir);

        QDir src(srcDir);
        const QStringList pngs = src.entryList(QStringList() << QStringLiteral("*.png"),
                                               QDir::Files);
        for(const QString& f : pngs)
        {
            const QString from = src.absoluteFilePath(f);
            const QString to = dstDir + f;
            if(QFile::exists(to))
                QFile::remove(to);
            if(QFile::copy(from, to))
                ok = true;
        }
    }

    QSet<QString> keysInHicolor;
    for(const QString& size : kNativeHicolorSizes)
    {
        QDir d(hicolorAppsDir(shortName, size));
        const QStringList pngs = d.entryList(QStringList() << QStringLiteral("*.png"),
                                              QDir::Files);
        for(const QString& f : pngs)
            keysInHicolor.insert(QFileInfo(f).completeBaseName());
    }

    for(const QString& zSize : kJollaSizes)
    {
        const QString jDir = packRoot + QStringLiteral("/jolla/") + zSize
                             + QStringLiteral("/icons");
        QDir jd(jDir);
        if(!jd.exists())
            continue;

        const QStringList pngs = jd.entryList(QStringList() << QStringLiteral("*.png"),
                                                QDir::Files);
        for(const QString& f : pngs)
        {
            const QString key = QFileInfo(f).completeBaseName();
            if(keysInHicolor.contains(key))
                continue;

            QImage img(jd.absoluteFilePath(f));
            if(img.isNull())
                continue;

            if(publishPngToAllHicolorSizes(shortName, key, img))
            {
                keysInHicolor.insert(key);
                ok = true;
            }
        }
    }

    QSet<QString> apkKeys;
    for(const QString& size : kApkPackSizes)
    {
        QDir d(packRoot + QStringLiteral("/apk/") + size);
        if(!d.exists())
            continue;
        const QStringList pngs = d.entryList(QStringList() << QStringLiteral("*.png"),
                                              QDir::Files);
        for(const QString& f : pngs)
            apkKeys.insert(QFileInfo(f).completeBaseName());
    }

    for(const QString& key : apkKeys)
    {
        if(publishApkKeyToHicolor(packName, key))
            ok = true;
    }

    return ok;
}

void IconApplier::removePackHicolorBridge(const QString& packName)
{
    const QString shortName = packShortName(packName);

    for(const QString& size : kNativeHicolorSizes)
    {
        const QString dstDir = QString::fromLatin1(kHicolorRoot) + QStringLiteral("/")
                               + size + QStringLiteral("/apps/") + shortName;
        QDir(dstDir).removeRecursively();
    }
}

QString IconApplier::nativeIconKey(const QString& iconValue, const QString& desktopPath) const
{
    QString key = iconKeyFromValue(iconValue);

    if(iconValue.contains(QLatin1Char('/')) && !iconValue.startsWith(QLatin1Char('/')))
    {
        const int slash = iconValue.indexOf(QLatin1Char('/'));
        const QString tail = iconValue.mid(slash + 1);
        if(!tail.isEmpty())
            key = iconKeyFromValue(tail);
    }

    if(key.isEmpty())
        key = baseForNative(desktopPath);

    return key;
}

QString IconApplier::themedIconId(const QString& packName, const QString& iconKey) const
{
    if(iconKey.isEmpty())
        return QString();
    return packShortName(packName) + QLatin1Char('/') + iconKey;
}

bool IconApplier::themedIconExists(const QString& packName, const QString& themedIcon) const
{
    if(themedIcon.startsWith(QLatin1Char('/')))
        return QFileInfo::exists(themedIcon);

    const int slash = themedIcon.indexOf(QLatin1Char('/'));
    if(slash <= 0)
        return false;

    if(themedIcon.left(slash) != packShortName(packName))
        return false;

    return hicolorHasIcon(themedIcon.left(slash), themedIcon.mid(slash + 1));
}

QString IconApplier::findNativeIcon(const QString& packName, const QString& iconValue) const
{
    const QString key = iconKeyFromValue(iconValue);
    if(key.isEmpty())
        return QString();

    const QString root = packDir(packName);

    for(const QString& s : kNativeHicolorSizes)
    {
        const QString p = root + QStringLiteral("/native/") + s
                          + QStringLiteral("/apps/") + key + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }

    for(const QString& s : kJollaSizes)
    {
        const QString p = root + QStringLiteral("/jolla/") + s + QStringLiteral("/icons/")
                          + key + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }

    return QString();
}

QString IconApplier::findNativeIconForDesktop(const QString& packName,
                                              const QString& iconValue,
                                              const QString& desktopPath) const
{
    QString themed = findNativeIcon(packName, iconValue);
    if(!themed.isEmpty())
        return themed;

    const QString base = baseForNative(desktopPath);
    const QString lookup = iconKeyFromValue(iconValue);
    if(lookup != base)
        themed = findNativeIcon(packName, base);
    return themed;
}

QString IconApplier::findApkIcon(const QString& packName, const QString& base) const
{
    const QString root = packDir(packName);
    for(const QString& s : kApkPackSizes)
    {
        const QString p = root + QStringLiteral("/apk/") + s + QStringLiteral("/")
                          + base + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }

    static const QString kPrefix = QStringLiteral("apkd_launcher_");
    if(base.startsWith(kPrefix))
    {
        QString legacyTail = base.mid(kPrefix.size());
        legacyTail.replace(QLatin1Char('_'), QLatin1Char('.'));
        const QString legacy = kPrefix + legacyTail;
        if(legacy != base)
        {
            for(const QString& s : kApkPackSizes)
            {
                const QString p = root + QStringLiteral("/apk/") + s + QStringLiteral("/")
                                  + legacy + QStringLiteral(".png");
                if(QFileInfo::exists(p))
                    return p;
            }
        }
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
    QString v = iconKeyFromValue(iconValue);

    static const QString kPrefix = QStringLiteral("apkd_launcher_");
    if(v.startsWith(kPrefix))
    {
        const int dash = v.indexOf(QLatin1Char('-'), kPrefix.size());
        if(dash > 0)
            v = v.left(dash);
    }
    return v;
}

int IconApplier::nativeMatchCount(const QString& packName) const
{
    if(packName.isEmpty())
        return 0;

    int count = 0;
    for(const QString& d : nativeDesktops())
    {
        DesktopFile df(d);
        if(!df.load())
            continue;
        const QString iv = df.value(QStringLiteral("Icon"));
        if(iv.isEmpty())
            continue;
        if(!findNativeIconForDesktop(packName, iv, d).isEmpty())
            ++count;
    }
    return count;
}

int IconApplier::apkMatchCount(const QString& packName) const
{
    if(packName.isEmpty())
        return 0;

    int count = 0;
    for(const QString& dpath : apkDesktops())
    {
        DesktopFile df(dpath);
        if(!df.load())
            continue;
        const QString iv = df.value(QStringLiteral("Icon"));
        if(iv.isEmpty())
            continue;
        if(!findApkIcon(packName, baseForApk(iv)).isEmpty())
            ++count;
    }
    return count;
}

QString IconApplier::resolveSourceIcon(const QString& iconValue, const QString& kind) const
{
    if(iconValue.isEmpty())
        return QString();

    if(iconValue.startsWith(QLatin1Char('/')) && QFileInfo::exists(iconValue))
        return iconValue;

    const QString key = iconKeyFromValue(iconValue);

    if(kind == QStringLiteral("apk"))
    {
        const QString p = QStringLiteral("/home/defaultuser/.local/share/apkd-bridge/launcherIcon/")
                          + key + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }
    else
    {
        for(const QString& s : kNativeHicolorSizes)
        {
            const QString p = QString::fromLatin1(kHicolorRoot) + QStringLiteral("/") + s
                              + QStringLiteral("/apps/") + key + QStringLiteral(".png");
            if(QFileInfo::exists(p))
                return p;
        }
    }
    return QString();
}

QImage IconApplier::makeOverlayImage(const QString& packName, const QString& base,
                                     const QString& kind, const QString& sourceIcon) const
{
    Q_UNUSED(base);

    if(sourceIcon.isEmpty())
        return QImage();

    const QString overlayBase = ImageUtil::randomOverlayBase(packDir(packName));
    if(overlayBase.isEmpty())
        return QImage();

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
    return out.isNull() ? QImage() : out;
}

void IconApplier::publishOverlayIconsToHicolor(const QString& packName)
{
    const QString shortName = packShortName(packName);

    auto maybeOverlay = [&](const QString& dpath, const QString& kind, bool isApk) {
        DesktopFile df(dpath);
        if(!df.load())
            return;

        const QString original = df.value(QStringLiteral("Icon"));
        if(isApk && original.isEmpty())
            return;

        const bool hasPack = isApk
                                 ? !findApkIcon(packName, baseForApk(original)).isEmpty()
                                 : !findNativeIconForDesktop(packName, original, dpath).isEmpty();
        if(hasPack)
            return;

        const QString base = isApk ? baseForApk(original) : baseForNative(dpath);
        const QString iconKey = isApk ? base : nativeIconKey(original, dpath);

        const QString source = resolveSourceIcon(original, kind);
        if(source.isEmpty())
            return;

        const QImage img = makeOverlayImage(packName, base, kind, source);
        if(img.isNull())
            return;

        publishPngToAllHicolorSizes(shortName, iconKey, img);
    };

    for(const QString& dpath : nativeDesktops())
        maybeOverlay(dpath, QStringLiteral("native"), false);

    for(const QString& dpath : apkDesktops())
        maybeOverlay(dpath, QStringLiteral("apk"), true);
}

void IconApplier::applyIcons(const QString& packName, bool overlay)
{
    if(packName.isEmpty())
    {
        emit applied();
        return;
    }

    FileLock lk;
    Q_UNUSED(lk);

    {
        IconManifest mfRestore(manifestPath());
        if(mfRestore.load())
        {
            const QString oldPack = mfRestore.activeIconPack();
            if(!oldPack.isEmpty())
                removePackHicolorBridge(oldPack);

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

                df.setValue(QStringLiteral("Icon"), stripUiThemerSuffix(e.originalIcon));
                if(df.save())
                {
                    if(e.kind == QStringLiteral("apk"))
                        chownToDefaultUser(dpath);
                    mfRestore.removeEntry(dpath);
                }
            }
            mfRestore.setActiveIconPack(QString());
            mfRestore.save();
        }
    }

    if(!installPackHicolorBridge(packName))
        qWarning() << "uithemer: hicolor publish failed for" << packName;

    if(overlay)
        publishOverlayIconsToHicolor(packName);

    IconManifest manifest(manifestPath());
    manifest.load();
    manifest.setActiveIconPack(packName);

    const QString shortName = packShortName(packName);
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

        const QString iconKey = isApk ? baseForApk(original) : nativeIconKey(original, dpath);

        const bool hasPack = isApk
                                 ? !findApkIcon(packName, iconKey).isEmpty()
                                 : !findNativeIconForDesktop(packName, original, dpath).isEmpty();
        const bool hasOverlay = overlay && hicolorHasIcon(shortName, iconKey);

        if(!hasPack && !hasOverlay)
            return;

        const QString themed = themedIconId(packName, iconKey);
        if(themed.isEmpty())
            return;

        IconManifest::Entry e;
        e.originalIcon = original;
        e.themedIcon = themed;
        e.kind = kind;
        manifest.setEntry(dpath, e);
        manifest.save();

        df.setValue(QStringLiteral("Icon"), themed);
        if(!df.save())
        {
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

    const QString activePack = manifest.activeIconPack();
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
            manifest.removeEntry(dpath);
            continue;
        }

        if(!df.load())
            continue;

        df.setValue(QStringLiteral("Icon"), stripUiThemerSuffix(e.originalIcon));
        if(!df.save())
            continue;
        if(e.kind == QStringLiteral("apk"))
            chownToDefaultUser(dpath);

        manifest.removeEntry(dpath);
    }

    if(!activePack.isEmpty())
        removePackHicolorBridge(activePack);

    manifest.setActiveIconPack(QString());
    manifest.save();
    emit restored();
}

void IconApplier::reassertWithinLock(IconManifest& manifest, int& reasserted, int& removed)
{
    const QString pack = manifest.activeIconPack();
    const QHash<QString, IconManifest::Entry> entries = manifest.entries();

    for(auto it = entries.begin(); it != entries.end(); ++it)
    {
        const QString dpath = it.key();
        const IconManifest::Entry& e = it.value();

        DesktopFile df(dpath);
        if(!df.exists())
        {
            manifest.removeEntry(dpath);
            ++removed;
            continue;
        }
        if(!df.load())
            continue;

        const QString cur = df.value(QStringLiteral("Icon"));

        if(cur == e.originalIcon)
            continue;

        if(!themedIconExists(pack, e.themedIcon))
        {
            df.setValue(QStringLiteral("Icon"), stripUiThemerSuffix(e.originalIcon));
            if(df.save())
            {
                if(e.kind == QStringLiteral("apk"))
                    chownToDefaultUser(dpath);
                manifest.removeEntry(dpath);
                ++removed;
            }
            continue;
        }

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

    const QString pack = manifest.activeIconPack();
    if(pack.isEmpty())
    {
        emit reasserted();
        return;
    }

    installPackHicolorBridge(pack);

    int reassertedCount = 0;
    int removedCount = 0;
    reassertWithinLock(manifest, reassertedCount, removedCount);

    if(reassertedCount > 0 || removedCount > 0)
        manifest.save();

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
    const QHash<QString, IconManifest::Entry> entries = manifest.entries();

    for(auto it = entries.begin(); it != entries.end(); ++it)
    {
        const QString dpath = it.key();
        IconManifest::Entry e = it.value();

        DesktopFile df(dpath);
        if(!df.load())
            continue;

        const QString cur = df.value(QStringLiteral("Icon"));
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

    installPackHicolorBridge(pack);

    if(overlay)
        publishOverlayIconsToHicolor(pack);

    int reassertedCount = 0;
    int removedCount = 0;
    reassertWithinLock(manifest, reassertedCount, removedCount);

    if(manifest.activeIconPack().isEmpty())
    {
        if(reassertedCount > 0 || removedCount > 0)
            manifest.save();
        emit newDesktopsThemed(0);
        return;
    }

    QSet<QString> known;
    const QHash<QString, IconManifest::Entry> entries = manifest.entries();
    for(auto it = entries.begin(); it != entries.end(); ++it)
        known.insert(it.key());

    const QString shortName = packShortName(pack);
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

        const QString iconKey = isApk ? baseForApk(original) : nativeIconKey(original, dpath);

        const bool hasPack = isApk
                                 ? !findApkIcon(pack, iconKey).isEmpty()
                                 : !findNativeIconForDesktop(pack, original, dpath).isEmpty();
        const bool hasOverlay = overlay && hicolorHasIcon(shortName, iconKey);

        if(!hasPack && !hasOverlay)
            return;

        const QString themedIcon = themedIconId(pack, iconKey);
        if(themedIcon.isEmpty())
            return;

        IconManifest::Entry e;
        e.originalIcon = original;
        e.themedIcon = themedIcon;
        e.kind = kind;
        manifest.setEntry(dpath, e);
        manifest.save();

        df.setValue(QStringLiteral("Icon"), themedIcon);
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
        manifest.save();

    emit newDesktopsThemed(themed + reassertedCount);
}

void IconApplier::enableAutoTheming(bool enable)
{
    if(enable)
    {
        if(_watcher)
            return;

        _watcher = new QFileSystemWatcher(this);
        const QStringList dirs = {
            QStringLiteral("/usr/share/applications"),
            QStringLiteral("/home/defaultuser/.local/share/applications"),
        };
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
    _watchDebounce.start();
}

void IconApplier::debouncedRescan()
{
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
