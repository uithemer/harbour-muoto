#include "iconupdater.h"
#include "desktopfile.h"
#include "filewrite.h"
#include "iconbackup.h"
#include "iconjob.h"
#include "iconjobqueue.h"
#include "iconprovider.h"
#include "iconresolve.h"
#include "iconupdater_p.h"
#include "launchericonops.h"
#include "launchermanifest.h"
#include "launcherpaths.h"

#include <MDesktopEntry>
#include <MGConfItem>
#include <silicatheme.h>

#include <sys/stat.h>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QStandardPaths>

namespace {

QString storedIconPath(const QString& desktopPath)
{
    const QFileInfo info(desktopPath);
    MGConfItem dconf(LauncherPaths::savedIconKey(info.completeBaseName()));
    return dconf.value().toString();
}

void storeIconPath(const QString& desktopPath, const QString& iconPath)
{
    const QFileInfo info(desktopPath);
    MGConfItem dconf(LauncherPaths::savedIconKey(info.completeBaseName()));
    dconf.set(iconPath);
}

QString generateIconPath(const QString& desktopPath, bool stable)
{
    const QFileInfo info(desktopPath);
    if(stable)
    {
        // Dynamic entries redraw every 60 s. A timestamped path each tick meant
        // a new file, an Icon= rewrite and a full manifest rewrite 1440 times a
        // day -- 1440 chances to lose the .desktop or the manifest. Overwriting
        // one path leaves both untouched; only the PNG bytes and the desktop's
        // mtime change.
        return LauncherPaths::generatedIconsDir() + QLatin1Char('/')
               + info.completeBaseName() + QStringLiteral("-dyn.png");
    }
    // Include msecs so rapid rebuilds never collide on the same path.
    const QString stamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    return LauncherPaths::generatedIconsDir() + QLatin1Char('/') + info.completeBaseName()
           + QLatin1Char('-') + stamp + QStringLiteral(".png");
}

QString normalizePath(const QString& path)
{
    return QCryptographicHash::hash(path.toLatin1(), QCryptographicHash::Sha1).toHex();
}

QString getFileFingerprint(const QString& path)
{
    QByteArray ba = path.toLocal8Bit();
    struct stat sb;
    if(stat(ba.constData(), &sb))
        return QString();

    return QString::number(sb.st_ino, 16) + QString::number(sb.st_size, 16)
           + QString::number(sb.st_mtim.tv_sec, 16) + QString::number(sb.st_mtim.tv_nsec, 16);
}

QString getStoredFingerprint(const QString& path)
{
    MGConfItem dconf(LauncherPaths::fingerprintKey(normalizePath(path)));
    return dconf.value(QStringLiteral("<unknown>")).toString();
}

void storeFingerprint(const QString& path, const QString& fingerprint)
{
    MGConfItem dconf(LauncherPaths::fingerprintKey(normalizePath(path)));
    dconf.set(fingerprint);
}

bool isOurIcon(const QString& iconPath)
{
    return getFileFingerprint(iconPath) == getStoredFingerprint(iconPath);
}

bool backupIcon(const QString& iconPath)
{
    return IconBackup::create(iconPath);
}

bool restoreIcon(const QString& iconPath)
{
    return IconBackup::restore(iconPath);
}

bool touchFile(const QString& path)
{
    QFile file(path);
    if(!file.exists() || !file.open(QIODevice::Append))
        return false;
    return futimens(file.handle(), nullptr) == 0;
}

} // namespace

IconUpdaterPrivate::IconUpdaterPrivate(IconProvider* provider, const QString& desktopPath,
                                       IconUpdater::Mode mode)
    : provider(provider)
    , desktopPath(desktopPath)
    , forceRedirect(mode == IconUpdater::RedirectOnly || mode == IconUpdater::RedirectStable)
    , stablePath(mode == IconUpdater::RedirectStable)
{
    MDesktopEntry desktopEntry(desktopPath);
    QString iconRef = desktopEntry.icon();

    // After a prior redirect apply, Icon= points at a generated PNG. Resolve the
    // original ref so we can choose inplace (hicolor/apk bridge) correctly.
    if(LauncherPaths::isOurGeneratedIconPath(iconRef))
    {
        const QString stored = storedIconPath(desktopPath);
        if(!stored.isEmpty())
            iconRef = stored;
    }

    iconPath = IconResolve::resolveIconPath(iconRef);
    alienDalvikIcon = IconResolve::isApkBridgeIcon(iconPath);

    // APK bridge: always redirect (Lipstick caches absolute Icon= paths).
    // Raster hicolor: always inplace, across every size slot, overlay included.
    // Redirect is a trap for these entries: Lipstick's LauncherModel pins a
    // launcher item to a concrete hicolor PNG path whenever it processes the
    // entry while Icon= is a bare hicolor-resolvable name (add, restore, rpm
    // update), and from then on the item paints that file and ignores Icon=
    // rewrites entirely -- a redirected apply after any such moment is
    // invisible until a homescreen restart. Keeping Icon= as the name and
    // rewriting the slot bytes instead makes every desktop touch re-pin the
    // item (with a bumped cache serial), so the grid follows pinned or not.
    // Only the dynamic clock/calendar keep their stable redirect path; their
    // icons are jolla theme names, which never resolve to hicolor and so
    // never pin.
    if(alienDalvikIcon)
        forceRedirect = true;
    else if(IconResolve::isMonitoredIcon(iconPath) && !stablePath)
        forceRedirect = false;

    monitoredIcon = !forceRedirect && IconResolve::isMonitoredIcon(iconPath);
}

bool IconUpdaterPrivate::updateMonitoredIcon()
{
    if(iconPath.isEmpty())
    {
        qWarning() << "muoto-launcher: unresolved icon for" << desktopPath;
        return false;
    }

    // Write every existing raster slot, not just the resolved one. Which size
    // directory Lipstick pins an item to is its configuration, and image://theme
    // may pick yet another sibling -- the only state that renders pack art
    // everywhere is: all slots hold pack art.
    const QStringList slotPaths = IconResolve::hicolorSlotPaths(iconPath);
    if(slotPaths.isEmpty())
    {
        qWarning() << "muoto-launcher: no hicolor slots for" << desktopPath;
        return false;
    }

    QString originalIcon = storedIconPath(desktopPath);
    if(originalIcon.isEmpty())
    {
        const QString current = MDesktopEntry(desktopPath).icon();
        if(!LauncherPaths::isOurGeneratedIconPath(current))
            originalIcon = current;
    }

    // Pass 1: back up and record. Every slot that has (or gets) a stock backup
    // is entered in the manifest *before* any byte is written, so a crash mid
    // -apply can never leave a themed slot that restore does not know about.
    // The premature entry is harmless the other way around: restoring a slot
    // that never got themed writes its own stock backup over stock bytes.
    QList<LauncherManifestEntry> entries;
    QStringList writable;
    for(const QString& slot : slotPaths)
    {
        // Without a stock backup an inplace restore has nothing to put back, so
        // do not theme the slot at all rather than theme it irreversibly.
        if(!isOurIcon(slot))
        {
            if(!backupIcon(slot))
            {
                qWarning() << "muoto-launcher: no stock backup for" << slot << "- skipping slot";
                continue;
            }
        }
        else if(!IconBackup::exists(slot))
        {
            // Bytes are ours (fingerprint matches) but the backup vanished --
            // someone removed ~/.local/share/harbour-muoto. There is no stock
            // source to recreate it from; keep theming so packs still switch,
            // but say clearly that restore cannot recover this slot.
            qWarning() << "muoto-launcher: themed slot has no stock backup" << slot
                       << "- restore cannot recover it (reinstall the owning app to fix)";
        }

        LauncherManifestEntry entry;
        entry.desktop = desktopPath;
        entry.originalIcon = originalIcon;
        entry.themedPath = slot;
        entry.mode = QStringLiteral("inplace");
        entries.append(entry);
        writable.append(slot);
    }

    if(writable.isEmpty())
        return false;

    LauncherManifest::replaceEntriesForDesktop(desktopPath, entries);

    // Pass 2: render and write the bytes.
    int written = 0;
    for(const QString& slot : writable)
    {
        int size = IconResolve::hicolorSlotSize(slot);
        if(size <= 0)
            size = qRound(Silica::Theme::instance()->iconSizeLauncher());
        // Request while the backup exists so overlay can load stock from backup.
        const QImage icon = provider->requestImage(QSize(size, size));
        if(icon.isNull())
        {
            qWarning() << "muoto-launcher: could not render icon for" << desktopPath;
            continue;
        }

        // Encode first, then replace the bytes of the existing hicolor PNG. A
        // remove-and-rename would drop the file to defaultuser ownership (no
        // CAP_CHOWN to put it back) and leave the icon missing if it failed.
        QByteArray png;
        {
            QBuffer buffer(&png);
            buffer.open(QIODevice::WriteOnly);
            if(!icon.save(&buffer, "PNG") || png.isEmpty())
            {
                qWarning() << "muoto-launcher: could not encode icon for" << desktopPath;
                continue;
            }
        }

        if(!FileWrite::inPlace(slot, png))
        {
            qWarning() << "muoto-launcher: could not write icon" << slot;
            continue;
        }

        storeFingerprint(slot, getFileFingerprint(slot));
        ++written;
    }

    if(written == 0)
        return false;

    // Revert a prior redirect Icon= to the stock name only now, after the slots
    // already hold pack art: whenever Lipstick processes the change, the name
    // never resolves to stock bytes. The old generated PNG is left on disk for
    // the next job's reconcileGeneratedIcons() -- deleting a file Lipstick may
    // still be showing is how tiles used to freeze.
    {
        MDesktopEntry current(desktopPath);
        if(LauncherPaths::isOurGeneratedIconPath(current.icon()) && !originalIcon.isEmpty())
        {
            DesktopFile desktop(desktopPath);
            desktop.setIcon(originalIcon);
            if(!desktop.save())
                qWarning() << "muoto-launcher: could not revert Icon= for" << desktopPath;
        }
    }

    if(!touchFile(desktopPath))
        qWarning() << "muoto-launcher: could not touch" << desktopPath;

    return true;
}

void IconUpdaterPrivate::restoreMonitoredIcon()
{
    if(iconPath.isEmpty())
        return;

    bool restored = false;
    for(const QString& slot : IconResolve::hicolorSlotPaths(iconPath))
    {
        if(!isOurIcon(slot))
            continue;
        restoreIcon(slot);
        restored = true;
    }

    if(!restored)
        return;

    touchFile(desktopPath);
    LauncherManifest::removeEntryForDesktop(desktopPath);
}

bool IconUpdaterPrivate::updateNonMonitoredIcon()
{
    DesktopFile desktop(desktopPath);
    if(!desktop.loaded())
        return false;

    const QString currentIconPath = desktop.icon();
    const bool isOurIconPath = LauncherPaths::isOurGeneratedIconPath(currentIconPath);

    if(!isOurIconPath)
    {
        if(!currentIconPath.isEmpty())
            storeIconPath(desktopPath, currentIconPath);
    }

    // Prior inplace APK theming may have left a composited bridge PNG. Put stock
    // back before we point Icon= at a generated file, so restore stays correct.
    {
        const QString stockRef = isOurIconPath ? storedIconPath(desktopPath) : currentIconPath;
        const QString stockPath = IconResolve::resolveIconPath(stockRef);
        if(!stockPath.isEmpty() && IconBackup::exists(stockPath))
            restoreIcon(stockPath);
    }

    const QString newIconPath = generateIconPath(desktopPath, stablePath);
    QDir dir = QFileInfo(newIconPath).absoluteDir();
    if(!dir.mkpath(QStringLiteral(".")))
        return false;

    const int size = qRound(Silica::Theme::instance()->iconSizeLauncher());
    QImage newIcon = provider->requestImage(QSize(size, size));
    if(newIcon.isNull())
        return false;

    // Same discipline as the inplace path: stage, verify non-empty, then move
    // into place. A truncated PNG here is a blank tile on the grid.
    const QString newIconTmp = newIconPath + QStringLiteral(".muoto-write.png");
    QFile::remove(newIconTmp);
    if(!newIcon.save(newIconTmp, "PNG") || QFileInfo(newIconTmp).size() <= 0)
    {
        QFile::remove(newIconTmp);
        qWarning() << "muoto-launcher: could not stage icon for" << desktopPath;
        return false;
    }
    QFile::remove(newIconPath);
    if(!QFile::rename(newIconTmp, newIconPath))
    {
        QFile::remove(newIconTmp);
        qWarning() << "muoto-launcher: could not place icon at" << newIconPath;
        return false;
    }

    LauncherManifestEntry entry;
    entry.desktop = desktopPath;
    entry.originalIcon = isOurIconPath ? storedIconPath(desktopPath) : currentIconPath;
    entry.themedPath = newIconPath;
    entry.mode = QStringLiteral("redirect");
    if(!LauncherManifest::appendEntry(entry))
    {
        qWarning() << "muoto-launcher: manifest append failed for" << desktopPath;
        QFile::remove(newIconPath);
        return false;
    }

    desktop.setIcon(newIconPath);
    if(!desktop.save())
    {
        LauncherManifest::removeEntryForDesktop(desktopPath);
        QFile::remove(newIconPath);
        return false;
    }

    // Same-second rebuilds reuse the same path; never delete the file we just wrote.
    if(isOurIconPath && currentIconPath != newIconPath)
        QFile::remove(currentIconPath);

    touchFile(desktopPath);
    return true;
}

void IconUpdaterPrivate::restoreNonMonitoredIcon()
{
    const QString originalIcon = storedIconPath(desktopPath);
    if(originalIcon.isEmpty())
        return;

    DesktopFile desktop(desktopPath);
    if(!desktop.loaded())
        return;

    const QString currentIconPath = desktop.icon();
    if(!LauncherPaths::isOurGeneratedIconPath(currentIconPath))
        return;

    desktop.setIcon(originalIcon);
    desktop.save();
    QFile::remove(currentIconPath);

    // Undo any leftover inplace composite on the stock bridge/hicolor path.
    const QString stockPath = IconResolve::resolveIconPath(originalIcon);
    if(!stockPath.isEmpty() && IconBackup::exists(stockPath))
        restoreIcon(stockPath);

    LauncherManifest::removeEntryForDesktop(desktopPath);
}

bool IconUpdater::isThemedIconIntact(const QString& iconPath)
{
    return isOurIcon(iconPath);
}

IconUpdater::IconUpdater(IconProvider* provider, const QString& desktopPath,
                         QObject* parent, Mode mode)
    : QObject(parent)
    , d_ptr(new IconUpdaterPrivate(provider, desktopPath, mode))
{
    // The provider signal enqueues; it does not write. The dynamic clock fires
    // this every 60 s, and running a write straight off the timer is how a tick
    // landed inside another operation's re-arm. Deliberately not connected to
    // update(): the constructor's own update() below is the in-job write path,
    // and routing that through the queue would enqueue work behind the job
    // currently building this very updater.
    connect(provider, &IconProvider::imageUpdated, this, &IconUpdater::requestUpdate);

    // APK entries are refreshed as a batch by the RefreshApk job: a per-updater
    // update() cannot re-arm Lipstick's watch, so the Icon= it wrote would go
    // unread.

    update();
}

void IconUpdater::requestUpdate()
{
    IconJob job;
    job.kind = IconJob::RebuildDyn;
    IconJobQueue::instance()->enqueue(job);
}

// Inert on purpose. This used to restore the icon depending on a global flag on
// LauncherIconOps, which meant deleting an updater had file-writing side effects
// that depended on who happened to be deleting it -- and it never ran at process
// exit anyway, since the updaters are leaked there, so the behaviour was already
// inconsistent. Callers now say what they mean via restore().
IconUpdater::~IconUpdater() = default;

void IconUpdater::restore()
{
    if(d_ptr->monitoredIcon)
        d_ptr->restoreMonitoredIcon();
    else
        d_ptr->restoreNonMonitoredIcon();
}

void IconUpdater::update()
{
    if(d_ptr->provider.isNull())
    {
        qWarning() << "muoto-launcher: updater for" << d_ptr->desktopPath
                   << "has no provider; skipping";
        d_ptr->lastUpdateOk = false;
        return;
    }

    d_ptr->lastUpdateOk = d_ptr->monitoredIcon ? d_ptr->updateMonitoredIcon()
                                               : d_ptr->updateNonMonitoredIcon();
}

bool IconUpdater::lastUpdateOk() const
{
    return d_ptr->lastUpdateOk;
}
