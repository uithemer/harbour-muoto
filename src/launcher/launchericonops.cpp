#include "launchericonops.h"
#include "aliendalvikwatcher.h"
#include "dynamicicon.h"
#include "folderambient.h"
#include "harbourthemepack.h"
#include "iconpaths.h"
#include "iconresolve.h"
#include "iconupdater.h"
#include "launchermanifest.h"
#include "launchersettings.h"
#include "launcherpaths.h"
#include "launcherwatch.h"
#include "opstatus.h"
#include "overlayiconprovider.h"
#include "filelock.h"
#include "osupdateguard.h"

#include <MDesktopEntry>
#include <MGConfItem>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QFileSystemWatcher>
#include <QHash>
#include <QSize>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace {

QHash<QString, IconPack*> s_iconPacks;
QHash<QString, IconUpdater*> s_updaters;

// Long enough to outlast a slow apkd launcher-entry sync, short enough that a
// user who opens the homescreen right after a container restart sees it heal.
const int kApkVerifyDelayMs = 15000;

// rpm writes the desktop entry and its icons as separate events; wait for the
// transaction to settle before looking at what changed.
const int kDesktopScanDelayMs = 2000;

MGConfItem* activeIconPackConf()
{
    static auto* conf = new MGConfItem(QStringLiteral("/apps/harbour-muoto/activeIconPack"));
    return conf;
}

void mgconfSetBool(const char* path, bool value)
{
    MGConfItem item(QString::fromLatin1(path));
    item.set(value);
}

QStringList applicationsDirs()
{
    return {QStringLiteral("/usr/share/applications"),
            QStringLiteral("%1/applications")
                .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))};
}

QFileInfoList desktopEntries()
{
    QFileInfoList entries;
    for(const QString& dirPath : applicationsDirs())
        entries += QDir(dirPath).entryInfoList({QStringLiteral("*.desktop")}, QDir::Files);
    return entries;
}

// Mirrors IconUpdater's resolution: once a pack is applied Icon= names a
// generated PNG, so fall back to the saved original to classify the entry.
QString stockIconRef(const QString& desktopPath)
{
    const QString iconRef = MDesktopEntry(desktopPath).icon();
    if(!LauncherPaths::isOurGeneratedIconPath(iconRef))
        return iconRef;

    const QFileInfo info(desktopPath);
    MGConfItem dconf(LauncherPaths::savedIconKey(info.completeBaseName()));
    const QString stored = dconf.value().toString();
    return stored.isEmpty() ? iconRef : stored;
}

QStringList apkBridgeDesktops()
{
    QStringList paths;
    for(const QFileInfo& info : desktopEntries())
    {
        const QString desktopPath = info.absoluteFilePath();
        if(IconResolve::isApkBridgeIcon(IconResolve::resolveIconPath(stockIconRef(desktopPath))))
            paths.append(desktopPath);
    }
    return paths;
}

bool packHasOverlayAssets(const QString& packRoot)
{
    if(packRoot.isEmpty())
        return false;
    const QString overlayDir = IconPaths::resolvePackCapabilityDir(packRoot,
                                                                 QStringLiteral("overlay"));
    if(overlayDir.isEmpty())
        return false;
    return !QDir(overlayDir).entryList({QStringLiteral("*.png")}, QDir::Files).isEmpty();
}

QUrl providerUriForDesktop(const QString& desktopPath)
{
    const QFileInfo info(desktopPath);
    MGConfItem appConf(LauncherPaths::perAppProviderKey(info.completeBaseName()));
    const QString applicationProvider = appConf.value().toString();
    // Only dynamic-icon:// (clock/calendar) is still a per-app provider.
    // Ignore leftover icon-pack:// overrides from the retired Customize UI.
    if(!applicationProvider.isEmpty() && applicationProvider != QLatin1String("<none>"))
    {
        const QUrl uri(applicationProvider);
        if(uri.scheme() == QLatin1String("dynamic-icon"))
            return uri;
    }

    const QString iconPack = activeIconPackConf()->value(QStringLiteral("default")).toString();
    if(iconPack.isEmpty() || iconPack == QLatin1String("default"))
        return QUrl();

    return QUrl(QStringLiteral("icon-pack://") + iconPack);
}

bool hasPerAppProvider(const QString& desktopPath)
{
    const QFileInfo info(desktopPath);
    MGConfItem appConf(LauncherPaths::perAppProviderKey(info.completeBaseName()));
    const QString applicationProvider = appConf.value().toString();
    if(applicationProvider.isEmpty() || applicationProvider == QLatin1String("<none>"))
        return false;
    return QUrl(applicationProvider).scheme() == QLatin1String("dynamic-icon");
}

IconUpdater* createIconPackUpdater(const QString& name,
                                   const QString& desktopPath,
                                   const QString& iconId = QString())
{
    IconPack* iconPack = s_iconPacks.value(name, nullptr);
    if(!iconPack)
    {
        iconPack = HarbourThemePack::byShortName(name);
        if(iconPack)
            s_iconPacks.insert(iconPack->name(), iconPack);
    }
    if(!iconPack)
        return nullptr;

    QString resolvedId = iconId;
    if(resolvedId.isEmpty())
        resolvedId = iconPack->iconByDesktopPath(desktopPath);
    if(resolvedId.isEmpty())
        return nullptr;

    // Pack may advertise an id that fails to load — fall through to overlay.
    if(iconPack->requestIcon(resolvedId, QSize(1, 1)).isNull())
        return nullptr;

    return iconPack->iconUpdater(desktopPath, resolvedId);
}

IconUpdater* createDynamicIconUpdater(const QString& name)
{
    static QHash<QString, DynamicIcon*> dynamicIcons;
    static bool loaded = false;
    if(!loaded)
    {
        for(DynamicIcon* icon : loadDynamicIcons())
            dynamicIcons.insert(icon->name(), icon);
        loaded = true;
    }

    DynamicIcon* dynamicIcon = dynamicIcons.value(name, nullptr);
    if(!dynamicIcon || !dynamicIcon->available())
        return nullptr;

    if(name == QLatin1String("muoto-jolla-clock") && !LauncherSettings::dynamicClockEnabled())
        return nullptr;
    if(name == QLatin1String("muoto-jolla-calendar") && !LauncherSettings::dynamicCalendarEnabled())
        return nullptr;

    return dynamicIcon->enabled() ? dynamicIcon->iconUpdater() : nullptr;
}

IconUpdater* createOverlayUpdater(const QString& packRoot, const QString& desktopPath)
{
    auto* provider = new OverlayIconProvider(packRoot, desktopPath);
    return new IconUpdater(provider, desktopPath, nullptr, IconUpdater::RedirectOnly);
}

IconUpdater* createPackOrOverlayUpdater(const QString& packName, const QString& desktopPath,
                                        bool applyPack, const QString& iconId = QString())
{
    if(applyPack)
    {
        IconUpdater* updater = createIconPackUpdater(packName, desktopPath, iconId);
        if(updater)
            return updater;
    }

    const QString packRoot = IconPaths::packDir(packName);
    if(LauncherSettings::iconOverlay() && packHasOverlayAssets(packRoot))
        return createOverlayUpdater(packRoot, desktopPath);

    return nullptr;
}

IconUpdater* createIconUpdater(const QString& desktopPath)
{
    const QUrl uri = providerUriForDesktop(desktopPath);
    const QString scheme = uri.scheme();
    const bool perApp = hasPerAppProvider(desktopPath);
    const bool applyPack = LauncherIconOps::instance()->applyPackIcons() || perApp;

    if(scheme == QStringLiteral("icon-pack"))
    {
        QString iconId = uri.path();
        if(iconId.startsWith(QLatin1Char('/')))
            iconId = iconId.mid(1);

        return createPackOrOverlayUpdater(uri.host(), desktopPath, applyPack, iconId);
    }

    if(scheme == QStringLiteral("dynamic-icon"))
    {
        IconUpdater* dyn = createDynamicIconUpdater(uri.host());
        if(dyn)
            return dyn;

        const QString pack = activeIconPackConf()->value(QStringLiteral("default")).toString();
        if(pack.isEmpty() || pack == QLatin1String("default"))
            return nullptr;
        return createPackOrOverlayUpdater(pack, desktopPath, applyPack);
    }

    return nullptr;
}

QStringList visibleDesktopPaths()
{
    QStringList paths;
    for(const QFileInfo& info : desktopEntries())
    {
        const QString desktopPath = info.absoluteFilePath();
        if(MDesktopEntry(desktopPath).noDisplay())
            continue;
        paths.append(desktopPath);
    }
    return paths;
}

} // namespace

LauncherIconOps* LauncherIconOps::instance()
{
    static LauncherIconOps* s_instance = new LauncherIconOps();
    return s_instance;
}

LauncherIconOps::LauncherIconOps(QObject* parent)
    : QObject(parent)
{
    connect(AlienDalvikWatcher::instance(), &AlienDalvikWatcher::containerReady,
            this, [this]() { refreshApkIcons(true); });

    // An install writes several files; coalesce the burst into one pass.
    m_desktopScan.setSingleShot(true);
    m_desktopScan.setInterval(kDesktopScanDelayMs);
    connect(&m_desktopScan, &QTimer::timeout, this, &LauncherIconOps::refreshNewDesktops);

    m_desktopDirWatcher = new QFileSystemWatcher(this);
    connect(m_desktopDirWatcher, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString&) { m_desktopScan.start(); });
    ensureDesktopDirWatch();
}

void LauncherIconOps::reloadIconPacks()
{
    qDeleteAll(s_iconPacks);
    s_iconPacks.clear();

    const QString active = activeIconPackConf()->value(QStringLiteral("default")).toString();
    if(!active.isEmpty() && active != QLatin1String("default"))
    {
        IconPack* pack = HarbourThemePack::byShortName(active);
        if(pack)
            s_iconPacks.insert(pack->name(), pack);
    }
}

void LauncherIconOps::clearUpdaters(bool restoreOnDestroy)
{
    m_restoreOnUpdaterDestroy = restoreOnDestroy;
    qDeleteAll(s_updaters);
    s_updaters.clear();
    m_restoreOnUpdaterDestroy = true;
}

void LauncherIconOps::rebuildIconUpdaters()
{
    if(m_rebuilding)
    {
        qInfo() << "muoto-launcher: skip re-entrant rebuildIconUpdaters";
        return;
    }
    if(m_inIconOp)
    {
        qInfo() << "muoto-launcher: skip rebuildIconUpdaters during icon op";
        return;
    }
    rebuildIconUpdatersNow();
}

void LauncherIconOps::emitProgress(int done, int total)
{
    if(!m_inIconOp || total <= 0)
        return;

    // One signal per desktop entry would be ~100 D-Bus messages and as many
    // notification republishes on the GUI side; 5% steps animate a bar just
    // as well.
    const int percent = (done * 100) / total;
    if(done < total && percent - m_progressPercent < 5)
        return;

    m_progressPercent = percent;
    emit progress(done, total);
}

void LauncherIconOps::rearmApkDesktopWatches()
{
    // apkd rewrites apkd_launcher_*.desktop with rename(2) whenever Android
    // support regenerates them, and Lipstick never re-adds the inotify watch it
    // loses on the old inode. Any Icon= we write afterwards goes unnoticed, so
    // put the watches back before an apply or restore touches those entries.
    LauncherWatch::rearmDesktopWatches(apkBridgeDesktops());
}

void LauncherIconOps::rearmAllDesktopWatches()
{
    // Native RPM updates do the same rename(2) to /usr/share/applications, so
    // apply/restore have to re-arm those watches too — not only the APK ones.
    LauncherWatch::rearmDesktopWatches(visibleDesktopPaths());
}

bool LauncherIconOps::apkIconsClobbered() const
{
    // Only entries we actually themed count: an APK app the pack has no icon
    // for legitimately keeps its stock Icon=.
    for(const QString& desktopPath : apkBridgeDesktops())
    {
        if(!s_updaters.contains(desktopPath))
            continue;
        if(!LauncherPaths::isOurGeneratedIconPath(MDesktopEntry(desktopPath).icon()))
            return true;
    }
    return false;
}

void LauncherIconOps::refreshApkIcons(bool scheduleVerify)
{
    if(m_inIconOp || m_rebuilding)
    {
        qInfo() << "muoto-launcher: skip refreshApkIcons during icon op";
        return;
    }

    const QString active = activeIconPackConf()->value(QStringLiteral("default")).toString();
    if(active.isEmpty() || active == QLatin1String("default"))
        return;

    if(OsUpdateGuard::running())
    {
        qInfo() << "muoto-launcher: skip refreshApkIcons (upgrade in progress)";
        return;
    }

    FileLock lock(FileLock::defaultLockPath(), false);
    if(!lock.isHeld())
    {
        qInfo() << "muoto-launcher: skip refreshApkIcons (busy)";
        return;
    }

    const QStringList apkDesktops = apkBridgeDesktops();
    if(apkDesktops.isEmpty())
        return;

    m_inIconOp = true;
    rearmApkDesktopWatches();

    // Recreate rather than update() the existing updaters: apkd may have added
    // entries for Android apps installed while the container was down.
    m_restoreOnUpdaterDestroy = false;
    int themed = 0;
    for(const QString& desktopPath : apkDesktops)
    {
        delete s_updaters.take(desktopPath);
        if(IconUpdater* updater = createIconUpdater(desktopPath))
        {
            s_updaters.insert(desktopPath, updater);
            ++themed;
        }
    }
    m_restoreOnUpdaterDestroy = true;
    m_inIconOp = false;

    qInfo() << "muoto-launcher: refreshApkIcons pack=" << active
            << "desktops=" << apkDesktops.size() << "themed=" << themed;

    if(!scheduleVerify)
        return;

    // The gap between apkd rewriting the desktops and announcing containerReady
    // measured comfortable, but on one device only; re-check once so a slower
    // apkd costs a second pass rather than stock icons until the next apply.
    QTimer::singleShot(kApkVerifyDelayMs, this, [this]() {
        if(!apkIconsClobbered())
            return;
        qInfo() << "muoto-launcher: APK icons clobbered after refresh, retrying";
        refreshApkIcons(false);
    });
}

void LauncherIconOps::reconcileGeneratedIcons()
{
    QDir generated(LauncherPaths::generatedIconsDir());
    if(!generated.exists())
        return;

    // Two failure modes, one rule. Deleting a PNG a desktop still names gives
    // Lipstick inotify ENOENT and a frozen tile, so a partial restore must not
    // wipe the directory; and nothing ever removed superseded files, so they
    // accumulated. Keep exactly what something still points at.
    QSet<QString> referenced;
    for(const QFileInfo& info : desktopEntries())
    {
        const QString icon = MDesktopEntry(info.absoluteFilePath()).icon();
        if(LauncherPaths::isOurGeneratedIconPath(icon))
            referenced.insert(icon);
    }

    QList<LauncherManifestEntry> entries;
    if(LauncherManifest::load(&entries))
    {
        for(const LauncherManifestEntry& e : entries)
        {
            if(LauncherPaths::isOurGeneratedIconPath(e.themedPath))
                referenced.insert(e.themedPath);
        }
    }

    int removed = 0;
    const QStringList pngs = generated.entryList({QStringLiteral("*.png")}, QDir::Files);
    for(const QString& f : pngs)
    {
        const QString path = generated.absoluteFilePath(f);
        if(referenced.contains(path))
            continue;
        if(QFile::remove(path))
            ++removed;
    }

    if(removed > 0)
        qInfo() << "muoto-launcher: reclaimed" << removed << "unreferenced generated icons";
}

void LauncherIconOps::ensureDesktopDirWatch()
{
    if(!m_desktopDirWatcher)
        return;

    // A directory replaced wholesale takes its watch with it, so re-add after
    // every pass rather than only at startup.
    const QStringList watched = m_desktopDirWatcher->directories();
    for(const QString& dirPath : applicationsDirs())
    {
        if(watched.contains(dirPath) || !QDir(dirPath).exists())
            continue;
        m_desktopDirWatcher->addPath(dirPath);
    }
}

QStringList LauncherIconOps::desktopsNeedingTheme() const
{
    QStringList paths;

    // Installed while we were not looking: no updater attached yet.
    for(const QFileInfo& info : desktopEntries())
    {
        const QString desktopPath = info.absoluteFilePath();
        if(s_updaters.contains(desktopPath))
            continue;
        if(MDesktopEntry(desktopPath).noDisplay())
            continue;
        paths.append(desktopPath);
    }

    // Updated behind our back: rpm reinstalled the app and overwrote the
    // hicolor png we had composited in place.
    QList<LauncherManifestEntry> entries;
    LauncherManifest::load(&entries);
    for(const LauncherManifestEntry& entry : entries)
    {
        if(entry.mode != QLatin1String("inplace"))
            continue;
        if(paths.contains(entry.desktop) || !QFile::exists(entry.desktop))
            continue;
        if(IconUpdater::isThemedIconIntact(entry.themedPath))
            continue;
        paths.append(entry.desktop);
    }

    return paths;
}

void LauncherIconOps::refreshNewDesktops()
{
    if(m_inIconOp || m_rebuilding)
    {
        qInfo() << "muoto-launcher: skip refreshNewDesktops during icon op";
        return;
    }

    const QString active = activeIconPackConf()->value(QStringLiteral("default")).toString();
    if(active.isEmpty() || active == QLatin1String("default"))
        return;

    if(OsUpdateGuard::running())
        return;

    // Our own writes wake the watcher too; deciding there is nothing to do
    // before taking the lock keeps that the cheap path.
    const QStringList pending = desktopsNeedingTheme();
    if(pending.isEmpty())
    {
        ensureDesktopDirWatch();
        return;
    }

    FileLock lock(FileLock::defaultLockPath(), false);
    if(!lock.isHeld())
    {
        // An apply already in flight covers these entries anyway.
        qInfo() << "muoto-launcher: skip refreshNewDesktops (busy)";
        return;
    }

    m_inIconOp = true;
    // rpm install/update replaces the .desktop with rename(2), which drops
    // Lipstick's per-file watch. Re-arm before we write the themed PNG.
    LauncherWatch::rearmDesktopWatches(pending);
    m_restoreOnUpdaterDestroy = false;
    int themed = 0;
    for(const QString& desktopPath : pending)
    {
        delete s_updaters.take(desktopPath);
        if(IconUpdater* updater = createIconUpdater(desktopPath))
        {
            s_updaters.insert(desktopPath, updater);
            ++themed;
        }
    }
    m_restoreOnUpdaterDestroy = true;
    m_inIconOp = false;

    ensureDesktopWatches();
    ensureDesktopDirWatch();

    qInfo() << "muoto-launcher: refreshNewDesktops pack=" << active
            << "pending=" << pending.size() << "themed=" << themed;
}

void LauncherIconOps::rebuildIconUpdatersNow()
{
    m_rebuilding = true;
    m_updatersBuilt = 0;
    m_updatersWritten = 0;
    LauncherWatch::sweepStaleRearmFiles(applicationsDirs());
    // Restore previous redirects before re-attaching so toggling dyn off
    // (or leaving a pack) does not leave stale generated Icon= values.
    clearUpdaters(true);
    reloadIconPacks();

    const QString active = activeIconPackConf()->value(QStringLiteral("default")).toString();
    const bool packActive = !active.isEmpty() && active != QLatin1String("default");

    // With no pack, still attach dynamic clock/calendar updaters (stock SVG assets).
    QStringList desktopPaths;
    const QFileInfoList infoList = desktopEntries();
    m_progressTotal = infoList.size() + 1;
    m_progressPercent = -1;
    int examined = 0;
    for(const QFileInfo& info : infoList)
    {
        emitProgress(++examined, m_progressTotal);

        const QString desktopPath = info.absoluteFilePath();
        MDesktopEntry desktopEntry(desktopPath);
        if(desktopEntry.noDisplay())
            continue;

        desktopPaths.append(desktopPath);

        IconUpdater* updater = createIconUpdater(desktopPath);
        if(updater)
        {
            // Only entries we actually built an updater for count. A pack that
            // simply has no icon for an app yields no updater and is not a
            // failure, or every device would report one.
            ++m_updatersBuilt;
            if(updater->lastUpdateOk())
                ++m_updatersWritten;
            s_updaters.insert(desktopPath, updater);
        }
    }

    LauncherManifest::pruneOrphans(desktopPaths);
    ensureDesktopWatches();
    qInfo() << "muoto-launcher: rebuildIconUpdaters active="
            << (packActive ? active : QStringLiteral("<default>"))
            << "count=" << s_updaters.size();
    m_rebuilding = false;
}

void LauncherIconOps::ensureDesktopWatches()
{
    static QHash<QString, MGConfItem*> watches;

    const QFileInfoList infoList = desktopEntries();
    for(const QFileInfo& info : infoList)
    {
        const QString desktopPath = info.absoluteFilePath();
        if(watches.contains(desktopPath))
            continue;

        const QFileInfo desktopInfo(desktopPath);
        auto* conf = new MGConfItem(LauncherPaths::perAppProviderKey(desktopInfo.completeBaseName()), this);
        watches.insert(desktopPath, conf);
        connect(conf, &MGConfItem::valueChanged, this, &LauncherIconOps::rebuildIconUpdaters);
    }
}

void LauncherIconOps::applyIcons(const QString& pack, bool runPack, bool overlay)
{
    qInfo() << "muoto-launcher: ApplyIcons start pack=" << pack
            << "runPack=" << runPack << "overlay=" << overlay;

    if(OsUpdateGuard::running())
    {
        qInfo() << "muoto-launcher: ApplyIcons done ok=false msg=upgrade in progress updaters=0";
        emit applied(false, QStringLiteral("upgrade in progress"));
        return;
    }

    if(pack.isEmpty() || pack == QLatin1String("default"))
    {
        qInfo() << "muoto-launcher: ApplyIcons done ok=true msg=noop updaters=0";
        emit applied(true, QString());
        return;
    }

    const QString packRoot = IconPaths::packDir(pack);
    if(!QDir(packRoot).exists())
    {
        qInfo() << "muoto-launcher: ApplyIcons done ok=false msg=pack not found updaters=0";
        emit applied(false, QStringLiteral("pack not found"));
        return;
    }

    FileLock lock(FileLock::defaultLockPath(), false);
    if(!lock.isHeld())
    {
        qInfo() << "muoto-launcher: ApplyIcons done ok=false msg=busy updaters=0";
        emit applied(false, QStringLiteral("busy"));
        return;
    }

    m_inIconOp = true;
    rearmAllDesktopWatches();

    // Overlay styles apps missing from the pack — only valid with pack apply.
    if(overlay && !runPack)
        runPack = true;

    m_applyPackIcons = runPack;
    mgconfSetBool("/apps/harbour-muoto/iconOverlay", overlay);

    if(activeIconPackConf()->value(QStringLiteral("default")).toString() != pack)
        activeIconPackConf()->set(pack);

    rebuildIconUpdatersNow();
    FolderAmbient::apply(pack, overlay);
    reconcileGeneratedIcons();
    emitProgress(m_progressTotal, m_progressTotal);
    // Lipstick coalesces desktop events for 2s; don't tell the GUI we're done
    // until that holdback has expired or the success toast races the grid.
    LauncherWatch::waitForMonitorHoldback();
    m_inIconOp = false;

    const int built = m_updatersBuilt;
    const int written = m_updatersWritten;

    // Nothing written at all where work was expected means the writes are
    // failing wholesale -- most likely the daemon lost cap_dac_override on an
    // upgrade. Reporting success there is what hid this for so long.
    if(built > 0 && written == 0)
    {
        const QString msg = QStringLiteral("no icons could be written");
        qWarning() << "muoto-launcher: ApplyIcons done ok=false msg=" << msg
                   << "built=" << built;
        OpStatus::record(QStringLiteral("ApplyIcons"), OpStatus::HardFailure, msg, built, written);
        emit applied(false, msg);
        return;
    }

    if(written < built)
    {
        const QString msg = QStringLiteral("some icons could not be updated");
        qWarning() << "muoto-launcher: ApplyIcons done ok=partial built=" << built
                   << "written=" << written;
        // Partial is still an applied pack: the GUI says so, but update-icons
        // must not fail on it or the repair oneshot would never self-delete.
        OpStatus::record(QStringLiteral("ApplyIcons"), OpStatus::Partial, msg, built, written);
        emit applied(false, msg);
        return;
    }

    qInfo() << "muoto-launcher: ApplyIcons done ok=true msg= updaters=" << s_updaters.size();
    OpStatus::record(QStringLiteral("ApplyIcons"), OpStatus::Ok, QString(), built, written);
    emit applied(true, QString());
}

void LauncherIconOps::restoreIcons()
{
    qInfo() << "muoto-launcher: RestoreIcons start";

    FileLock lock(FileLock::defaultLockPath(), false);
    if(!lock.isHeld())
    {
        qInfo() << "muoto-launcher: RestoreIcons done ok=false msg=busy";
        emit restored(false, QStringLiteral("busy"));
        return;
    }

    m_inIconOp = true;
    // Restoring rewrites Icon= back to the stock value, which needs a live
    // watch just as much as applying does. Native RPM updates drop those
    // watches too, so re-arm every launcher desktop, not only APK.
    rearmAllDesktopWatches();
    clearUpdaters(false);

    const bool restoredOk = LauncherManifest::restoreAll();

    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir backup(QStringLiteral("%1/harbour-muoto/launcher-backup").arg(dataPath));

    // Only wipe backups after a fully successful restore — otherwise keep them
    // so a retry can still recover inplace icons.
    if(restoredOk && backup.exists())
        backup.removeRecursively();
    else if(!restoredOk)
        qWarning() << "muoto-launcher: keeping launcher-backup after partial restore failure";

    reconcileGeneratedIcons();

    FolderAmbient::restore();

    mgconfSetBool("/apps/harbour-muoto/iconOverlay", false);
    // Do not touch dyn clock/calendar flags. Callers that want them off
    // (Themes restore, uninstall, oneshot-restore) write dconf first.
    // Clearing them here retriggers rebuildIconUpdaters while the pack is
    // still active and backups are already gone, then QML cannot turn them
    // back on if ConfigurationGroup still reads true.
    activeIconPackConf()->set(QStringLiteral("default"));
    m_applyPackIcons = true;

    rebuildIconUpdatersNow();
    emitProgress(m_progressTotal, m_progressTotal);
    LauncherWatch::waitForMonitorHoldback();
    m_inIconOp = false;

    if(!restoredOk)
    {
        const QString msg = QStringLiteral("inplace restore failed");
        qInfo() << "muoto-launcher: RestoreIcons done ok=false msg=" << msg;
        OpStatus::record(QStringLiteral("RestoreIcons"), OpStatus::Partial, msg, 0, 0);
        emit restored(false, msg);
        return;
    }

    qInfo() << "muoto-launcher: RestoreIcons done ok=true msg=";
    OpStatus::record(QStringLiteral("RestoreIcons"), OpStatus::Ok, QString(), 0, 0);
    emit restored(true, QString());
}
