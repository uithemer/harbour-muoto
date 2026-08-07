#include "overlayiconprovider.h"
#include "iconresolve.h"
#include "launcherpaths.h"
#include "launchermanifest.h"
#include "overlayrender.h"

#include <MDesktopEntry>
#include <MGConfItem>
#include <silicatheme.h>

#include <QFile>
#include <QFileInfo>
#include <QImage>

namespace {

QString stockIconRefForDesktop(const QString& desktopPath)
{
    MDesktopEntry desktop(desktopPath);
    const QString current = desktop.icon();

    if(!LauncherPaths::isOurGeneratedIconPath(current))
        return current;

    const QFileInfo info(desktopPath);
    MGConfItem saved(LauncherPaths::savedIconKey(info.completeBaseName()));
    const QString stored = saved.value().toString();
    if(!stored.isEmpty() && !LauncherPaths::isOurGeneratedIconPath(stored))
        return stored;

    QList<LauncherManifestEntry> entries;
    if(LauncherManifest::load(&entries))
    {
        for(const LauncherManifestEntry& e : entries)
        {
            if(e.desktop != desktopPath)
                continue;
            if(!e.originalIcon.isEmpty()
               && !LauncherPaths::isOurGeneratedIconPath(e.originalIcon))
                return e.originalIcon;
        }
    }

    return QString();
}

} // namespace

OverlayIconProvider::OverlayIconProvider(const QString& packRoot,
                                         const QString& desktopPath,
                                         QObject* parent)
    : IconProvider(parent)
    , m_packRoot(packRoot)
    , m_desktopPath(desktopPath)
{
}

QImage OverlayIconProvider::requestImage(const QSize& requestedSize)
{
    const QString iconRef = stockIconRefForDesktop(m_desktopPath);
    QString stockPath = IconResolve::resolveIconPath(iconRef);
    if(stockPath.isEmpty() || LauncherPaths::isOurGeneratedIconPath(stockPath))
        return {};

    // After inplace theming the live path is already composited; use the backup.
    const QString backupPath = LauncherPaths::iconBackupPath(stockPath);
    if(QFile::exists(backupPath))
        stockPath = backupPath;

    const QString overlayBasePath = OverlayRender::overlayBaseForDesktop(m_packRoot, m_desktopPath);
    if(overlayBasePath.isEmpty())
        return {};

    QImage stock(stockPath);
    if(stock.isNull())
        return {};
    stock = stock.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    QImage overlayBase(overlayBasePath);
    if(overlayBase.isNull())
        return {};
    overlayBase = overlayBase.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    if(IconResolve::isAlienDalvikIcon(stockPath)
       || IconResolve::isAlienDalvikIcon(IconResolve::resolveIconPath(iconRef)))
    {
        const QSize outer(192, 192);
        const QSize inner(122, 122);
        return OverlayRender::composite(overlayBase, stock, outer, inner)
            .scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    const int launcherSize = qRound(Silica::Theme::instance()->iconSizeLauncher());
    const QSize outer(launcherSize, launcherSize);
    const QSize inner(int(launcherSize * 0.6), int(launcherSize * 0.6));
    return OverlayRender::composite(overlayBase, stock, outer, inner)
        .scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
