#include "launchericonops.h"
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
#include "overlayiconprovider.h"
#include "filelock.h"
#include "osupdateguard.h"

#include <MDesktopEntry>
#include <MGConfItem>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QHash>
#include <QSize>
#include <QStandardPaths>
#include <QUrl>

namespace {

QHash<QString, IconPack*> s_iconPacks;
QHash<QString, IconUpdater*> s_updaters;

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

} // namespace

LauncherIconOps* LauncherIconOps::instance()
{
    static LauncherIconOps* s_instance = new LauncherIconOps();
    return s_instance;
}

LauncherIconOps::LauncherIconOps(QObject* parent)
    : QObject(parent)
{
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
    rearmApkDesktopWatches();

    // Overlay styles apps missing from the pack — only valid with pack apply.
    if(overlay && !runPack)
        runPack = true;

    m_applyPackIcons = runPack;
    mgconfSetBool("/apps/harbour-muoto/iconOverlay", overlay);

    if(activeIconPackConf()->value(QStringLiteral("default")).toString() != pack)
        activeIconPackConf()->set(pack);

    rebuildIconUpdatersNow();
    FolderAmbient::apply(pack, overlay);
    emitProgress(m_progressTotal, m_progressTotal);
    m_inIconOp = false;
    qInfo() << "muoto-launcher: ApplyIcons done ok=true msg= updaters=" << s_updaters.size();
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
    // watch just as much as applying does.
    rearmApkDesktopWatches();
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
    m_inIconOp = false;

    if(!restoredOk)
    {
        qInfo() << "muoto-launcher: RestoreIcons done ok=false msg=inplace restore failed";
        emit restored(false, QStringLiteral("inplace restore failed"));
        return;
    }

    qInfo() << "muoto-launcher: RestoreIcons done ok=true msg=";
    emit restored(true, QString());
}
