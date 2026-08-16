#include "iconupdater.h"
#include "desktopentry.h"
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

QString generateIconPath(const QString& desktopPath)
{
    const QFileInfo info(desktopPath);
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

QString getIconBackupPath(const QString& iconPath)
{
    return LauncherPaths::iconBackupPath(iconPath);
}

void backupIcon(const QString& iconPath)
{
    const QString iconBackupPath = getIconBackupPath(iconPath);
    QFileInfo(iconBackupPath).dir().mkpath(QStringLiteral("."));
    QFile::remove(iconBackupPath);
    QFile::copy(iconPath, iconBackupPath);
}

void restoreIcon(const QString& iconPath)
{
    const QString iconBackupPath = getIconBackupPath(iconPath);
    QFile::remove(iconPath);
    QFile::copy(iconBackupPath, iconPath);
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
    , forceRedirect(mode == IconUpdater::RedirectOnly)
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
    // Hicolor with a single size slot: inplace (Icon=harbour-* unchanged).
    // Multi-size hicolor: redirect — inplace only rewrites one slot while
    // image://theme/ may paint another sibling (looks stock on the grid).
    if(alienDalvikIcon)
        forceRedirect = true;
    else if(IconResolve::isMonitoredIcon(iconPath))
        forceRedirect = IconResolve::hasAlternateHicolor(iconPath);

    monitoredIcon = !forceRedirect && IconResolve::isMonitoredIcon(iconPath);
}

void IconUpdaterPrivate::updateMonitoredIcon()
{
    if(iconPath.isEmpty())
    {
        qWarning() << "muoto-launcher: unresolved icon for" << desktopPath;
        return;
    }

    // Revert a prior redirect Icon= so Lipstick uses the inplace path again.
    {
        MDesktopEntry current(desktopPath);
        const QString currentIcon = current.icon();
        if(LauncherPaths::isOurGeneratedIconPath(currentIcon))
        {
            const QString orig = storedIconPath(desktopPath);
            if(!orig.isEmpty())
            {
                DesktopEntry desktop(desktopPath);
                desktop.setIcon(orig);
                if(desktop.save())
                    QFile::remove(currentIcon);
            }
        }
    }

    if(!isOurIcon(iconPath))
        backupIcon(iconPath);

    const int size = qRound(Silica::Theme::instance()->iconSizeLauncher());
    // Request while backup exists so overlay can load stock from backup or live path.
    QImage icon = provider->requestImage(QSize(size, size));
    if(icon.isNull())
    {
        qWarning() << "muoto-launcher: could not render icon for" << desktopPath;
        return;
    }

    const QString tmpPath = iconPath + QStringLiteral(".muoto-write.png");
    QFile::remove(tmpPath);
    // Explicit PNG: QImage::save() keys off the suffix; ".muoto-write" alone fails.
    if(!icon.save(tmpPath, "PNG") || QFileInfo(tmpPath).size() <= 0)
    {
        QFile::remove(tmpPath);
        qWarning() << "muoto-launcher: could not save icon to" << iconPath;
        return;
    }
    QFile::remove(iconPath);
    if(!QFile::rename(tmpPath, iconPath))
    {
        const bool copied = QFile::copy(tmpPath, iconPath);
        QFile::remove(tmpPath);
        if(!copied)
        {
            qWarning() << "muoto-launcher: could not replace icon" << iconPath;
            return;
        }
    }

    if(QFileInfo(iconPath).size() <= 0)
    {
        qWarning() << "muoto-launcher: refusing empty icon at" << iconPath;
        QFile::remove(iconPath);
        return;
    }

    if(!touchFile(desktopPath))
        qWarning() << "muoto-launcher: could not touch" << desktopPath;

    storeFingerprint(iconPath, getFileFingerprint(iconPath));

    LauncherManifestEntry entry;
    entry.desktop = desktopPath;
    entry.originalIcon = storedIconPath(desktopPath);
    if(entry.originalIcon.isEmpty())
        entry.originalIcon = MDesktopEntry(desktopPath).icon();
    entry.themedPath = iconPath;
    entry.mode = QStringLiteral("inplace");
    LauncherManifest::appendEntry(entry);
}

void IconUpdaterPrivate::restoreMonitoredIcon()
{
    if(iconPath.isEmpty() || !isOurIcon(iconPath))
        return;

    restoreIcon(iconPath);
    touchFile(desktopPath);
    LauncherManifest::removeEntryForDesktop(desktopPath);
}

void IconUpdaterPrivate::updateNonMonitoredIcon()
{
    DesktopEntry desktop(desktopPath);

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
        if(!stockPath.isEmpty() && QFile::exists(getIconBackupPath(stockPath)))
            restoreIcon(stockPath);
    }

    const QString newIconPath = generateIconPath(desktopPath);
    QDir dir = QFileInfo(newIconPath).absoluteDir();
    if(!dir.mkpath(QStringLiteral(".")))
        return;

    const int size = qRound(Silica::Theme::instance()->iconSizeLauncher());
    QImage newIcon = provider->requestImage(QSize(size, size));
    if(newIcon.isNull() || !newIcon.save(newIconPath, "PNG"))
        return;

    LauncherManifestEntry entry;
    entry.desktop = desktopPath;
    entry.originalIcon = isOurIconPath ? storedIconPath(desktopPath) : currentIconPath;
    entry.themedPath = newIconPath;
    entry.mode = QStringLiteral("redirect");
    if(!LauncherManifest::appendEntry(entry))
    {
        qWarning() << "muoto-launcher: manifest append failed for" << desktopPath;
        QFile::remove(newIconPath);
        return;
    }

    desktop.setIcon(newIconPath);
    if(!desktop.save())
    {
        LauncherManifest::removeEntryForDesktop(desktopPath);
        QFile::remove(newIconPath);
        return;
    }

    // Same-second rebuilds reuse the same path; never delete the file we just wrote.
    if(isOurIconPath && currentIconPath != newIconPath)
        QFile::remove(currentIconPath);

    touchFile(desktopPath);
}

void IconUpdaterPrivate::restoreNonMonitoredIcon()
{
    const QString originalIcon = storedIconPath(desktopPath);
    if(originalIcon.isEmpty())
        return;

    DesktopEntry desktop(desktopPath);
    const QString currentIconPath = desktop.icon();
    if(!LauncherPaths::isOurGeneratedIconPath(currentIconPath))
        return;

    desktop.setIcon(originalIcon);
    desktop.save();
    QFile::remove(currentIconPath);

    // Undo any leftover inplace composite on the stock bridge/hicolor path.
    const QString stockPath = IconResolve::resolveIconPath(originalIcon);
    if(!stockPath.isEmpty() && QFile::exists(getIconBackupPath(stockPath)))
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
    // Dyn ticks (clock/calendar) must not write while Apply/restore is mid
    // re-arm — enqueue a coalesced update instead. The synchronous update()
    // below still runs when a job is constructing this updater.
    connect(provider, &IconProvider::imageUpdated, this, [desktopPath]() {
        LauncherIconOps::instance()->enqueueRebuildDyn(desktopPath);
    });

    // APK entries are refreshed as a batch by LauncherIconOps::refreshApkIcons:
    // a per-updater update() cannot re-arm Lipstick's watch, so the Icon= it
    // wrote would go unread.

    update();
}

IconUpdater::~IconUpdater()
{
    if(!LauncherIconOps::instance()->restoreOnUpdaterDestroy())
        return;

    if(d_ptr->monitoredIcon)
        d_ptr->restoreMonitoredIcon();
    else
        d_ptr->restoreNonMonitoredIcon();
}

void IconUpdater::update()
{
    if(d_ptr->monitoredIcon)
        d_ptr->updateMonitoredIcon();
    else
        d_ptr->updateNonMonitoredIcon();
}
