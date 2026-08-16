#include "launchericonops.h"
#include "aliendalvikwatcher.h"
#include "dynamicicon.h"
#include "folderambient.h"
#include "harbourthemepack.h"
#include "iconjobqueue.h"
#include "iconpaths.h"
#include "iconresolve.h"
#include "iconupdater.h"
#include "launchermanifest.h"
#include "launchersettings.h"
#include "launcherpaths.h"
#include "launcherwatch.h"
#include "overlayiconprovider.h"
#include "osupdateguard.h"

#include <MDesktopEntry>
#include <MGConfItem>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFileSystemWatcher>
#include <QHash>
#include <QSet>
#include <QSize>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace {

QHash<QString, IconPack*> s_iconPacks;
QHash<QString, IconUpdater*> s_updaters;

// Desktops seen at the last rebuild. Brand-new names (not in this set) already
// have a Lipstick watch from addPaths(); renaming them during Apply is the
// two-app shuffle. Paths in this set may need re-arm after rename(2) even when
// they have no updater yet (first Apply from pack=default).
QSet<QString> s_knownDesktops;

// Long enough to outlast a slow apkd launcher-entry sync, short enough that a
// user who opens the homescreen right after a container restart sees it heal.
const int kApkVerifyDelayMs = 15000;

// Trailing debounce: each directoryChanged restarts this timer. Wait past
// Lipstick's 2 s holdback so a brand-new .desktop is a real launcher item
// before RefreshDesktops writes Icon= (a write during pending-add is dropped).
const int kDesktopScanDelayMs = 3000;

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

bool isDynamicDesktopBase(const QString& baseName)
{
    return baseName == QLatin1String("jolla-clock")
        || baseName == QLatin1String("jolla-calendar");
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
    , m_queue(new IconJobQueue(this, this))
{
    connect(AlienDalvikWatcher::instance(), &AlienDalvikWatcher::containerReady,
            this, [this]() { m_queue->enqueueRefreshApk(true); });

    // An install writes several files; coalesce the burst into one pass.
    m_desktopScan.setSingleShot(true);
    m_desktopScan.setInterval(kDesktopScanDelayMs);
    connect(&m_desktopScan, &QTimer::timeout, this,
            [this]() { m_queue->enqueueRefreshDesktops(); });

    m_desktopDirWatcher = new QFileSystemWatcher(this);
    connect(m_desktopDirWatcher, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString&) { m_desktopScan.start(); });
    ensureDesktopDirWatch();
}

bool LauncherIconOps::isJobRunning() const
{
    return m_queue && m_queue->isRunning();
}

bool LauncherIconOps::hasUpdater(const QString& desktopPath) const
{
    return s_updaters.contains(desktopPath);
}

void LauncherIconOps::enqueueRebuildDyn(const QString& desktopPath)
{
    if(m_queue)
        m_queue->enqueueRebuildDyn(desktopPath);
}

void LauncherIconOps::prepareShutdown()
{
    LauncherWatch::abortAndRecover(applicationsDirs());
}

void LauncherIconOps::emitApplyFinished(bool ok, const QString& message)
{
    emit applied(ok, message);
}

void LauncherIconOps::emitRestoreFinished(bool ok, const QString& message)
{
    emit restored(ok, message);
}

void LauncherIconOps::finishJob()
{
    if(m_queue)
        m_queue->jobFinished();
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
    // Apply / Restore already end in a full rebuild; ignore the dconf echo.
    if(m_queue && m_queue->isRunning() && m_queue->runningJobEndsInFullRebuild())
        return;
    if(m_queue)
        m_queue->enqueueRebuild();
}

void LauncherIconOps::emitProgress(int done, int total)
{
    if(!isJobRunning() || total <= 0)
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

QStringList LauncherIconOps::desktopsNeedingWatchRearm(const QStringList& candidates) const
{
    // Never aside/back clock or calendar — dyn ticks rewrite them every 60s and
    // a mid-re-arm write used to stub the desktop. Skip filenames that appeared
    // since the last rebuild (not in s_knownDesktops): Lipstick already called
    // addPaths(), and renaming them during its holdback shuffles the grid.
    QStringList paths;
    paths.reserve(candidates.size());
    for(const QString& desktopPath : candidates)
    {
        const QString base = QFileInfo(desktopPath).completeBaseName();
        if(isDynamicDesktopBase(base))
            continue;
        if(!s_knownDesktops.contains(desktopPath))
            continue;
        paths.append(desktopPath);
    }
    return paths;
}

void LauncherIconOps::rearmThen(const QStringList& candidates, const std::function<void()>& next)
{
    const QStringList toRearm = desktopsNeedingWatchRearm(candidates);
    LauncherWatch::rearmDesktopWatches(toRearm, next);
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

void LauncherIconOps::runRebuild()
{
    rebuildIconUpdatersNow();
    finishJob();
}

void LauncherIconOps::runRebuildDyn(const QStringList& desktopPaths)
{
    for(const QString& desktopPath : desktopPaths)
    {
        IconUpdater* updater = s_updaters.value(desktopPath, nullptr);
        if(updater)
            updater->update();
    }
    finishJob();
}

void LauncherIconOps::runRefreshApkIcons(bool scheduleVerify)
{
    const QString active = activeIconPackConf()->value(QStringLiteral("default")).toString();
    if(active.isEmpty() || active == QLatin1String("default"))
    {
        finishJob();
        return;
    }

    if(OsUpdateGuard::running())
    {
        qInfo() << "muoto-launcher: skip refreshApkIcons (upgrade in progress)";
        finishJob();
        return;
    }

    const QStringList apkDesktops = apkBridgeDesktops();
    if(apkDesktops.isEmpty())
    {
        finishJob();
        return;
    }

    rearmThen(apkDesktops, [this, active, apkDesktops, scheduleVerify]() {
        // Recreate rather than update() the existing updaters: apkd may have
        // added entries for Android apps installed while the container was down.
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

        qInfo() << "muoto-launcher: refreshApkIcons pack=" << active
                << "desktops=" << apkDesktops.size() << "themed=" << themed;

        finishJob();

        if(!scheduleVerify)
            return;

        // The gap between apkd rewriting the desktops and announcing
        // containerReady measured comfortable, but on one device only; re-check
        // once so a slower apkd costs a second pass rather than stock icons.
        QTimer::singleShot(kApkVerifyDelayMs, this, [this]() {
            if(!apkIconsClobbered())
                return;
            qInfo() << "muoto-launcher: APK icons clobbered after refresh, retrying";
            m_queue->enqueueRefreshApk(false);
        });
    });
}

void LauncherIconOps::runRefreshNewDesktops()
{
    const QString active = activeIconPackConf()->value(QStringLiteral("default")).toString();
    if(active.isEmpty() || active == QLatin1String("default"))
    {
        ensureDesktopDirWatch();
        finishJob();
        return;
    }

    if(OsUpdateGuard::running())
    {
        finishJob();
        return;
    }

    const QStringList pending = desktopsNeedingTheme();
    if(pending.isEmpty())
    {
        ensureDesktopDirWatch();
        finishJob();
        return;
    }

    // Re-arm only updates (already have an updater). Brand-new installs skip
    // re-arm — Lipstick already watches the new filename.
    rearmThen(pending, [this, active, pending]() {
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

        ensureDesktopWatches();
        ensureDesktopDirWatch();

        qInfo() << "muoto-launcher: refreshNewDesktops pack=" << active
                << "pending=" << pending.size() << "themed=" << themed;
        finishJob();
    });
}

void LauncherIconOps::rebuildIconUpdatersNow()
{
    m_rebuilding = true;
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
            s_updaters.insert(desktopPath, updater);
    }

    LauncherManifest::pruneOrphans(desktopPaths);
    ensureDesktopWatches();
    s_knownDesktops.clear();
    for(const QString& path : desktopPaths)
        s_knownDesktops.insert(path);
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

    m_queue->enqueueApply(pack, runPack, overlay);
}

void LauncherIconOps::runApplyIcons(const QString& pack, bool runPack, bool overlay)
{
    rearmThen(visibleDesktopPaths(), [this, pack, runPack, overlay]() {
        bool usePack = runPack;
        // Overlay styles apps missing from the pack — only valid with pack apply.
        if(overlay && !usePack)
            usePack = true;

        m_applyPackIcons = usePack;
        mgconfSetBool("/apps/harbour-muoto/iconOverlay", overlay);

        if(activeIconPackConf()->value(QStringLiteral("default")).toString() != pack)
            activeIconPackConf()->set(pack);

        rebuildIconUpdatersNow();
        FolderAmbient::apply(pack, overlay);
        emitProgress(m_progressTotal, m_progressTotal);

        // Lipstick coalesces desktop events for 2s; don't tell the GUI we're done
        // until that holdback has expired or the success toast races the grid.
        LauncherWatch::waitForMonitorHoldback([this]() {
            qInfo() << "muoto-launcher: ApplyIcons done ok=true msg= updaters=" << s_updaters.size();
            emit applied(true, QString());
            finishJob();
        });
    });
}

void LauncherIconOps::restoreIcons()
{
    qInfo() << "muoto-launcher: RestoreIcons start";
    m_queue->enqueueRestore();
}

void LauncherIconOps::runRestoreIcons()
{
    rearmThen(visibleDesktopPaths(), [this]() {
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

        QDir generated(LauncherPaths::generatedIconsDir());
        if(generated.exists())
        {
            const QStringList pngs = generated.entryList({QStringLiteral("*.png")}, QDir::Files);
            for(const QString& f : pngs)
                QFile::remove(generated.absoluteFilePath(f));
        }

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
        LauncherWatch::waitForMonitorHoldback([this, restoredOk]() {
            if(!restoredOk)
            {
                qInfo() << "muoto-launcher: RestoreIcons done ok=false msg=inplace restore failed";
                emit restored(false, QStringLiteral("inplace restore failed"));
            }
            else
            {
                qInfo() << "muoto-launcher: RestoreIcons done ok=true msg=";
                emit restored(true, QString());
            }
            finishJob();
        });
    });
}
