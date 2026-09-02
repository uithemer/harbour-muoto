#include "folderambient.h"

#include "filewrite.h"
#include "iconpaths.h"
#include "overlayrender.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>

namespace FolderAmbient {

namespace {

const QStringList& folderIconNames()
{
    static const QStringList names = {
        QStringLiteral("icon-launcher-folder-01"),
        QStringLiteral("icon-launcher-folder-02"),
        QStringLiteral("icon-launcher-folder-03"),
        QStringLiteral("icon-launcher-folder-04"),
        QStringLiteral("icon-launcher-folder-05"),
        QStringLiteral("icon-launcher-folder-06"),
        QStringLiteral("icon-launcher-folder-07"),
        QStringLiteral("icon-launcher-folder-08"),
        QStringLiteral("icon-launcher-folder-09"),
        QStringLiteral("icon-launcher-folder-10"),
        QStringLiteral("icon-launcher-folder-11"),
        QStringLiteral("icon-launcher-folder-12"),
        QStringLiteral("icon-launcher-folder-13"),
        QStringLiteral("icon-launcher-folder-14"),
        QStringLiteral("icon-launcher-folder-15"),
        QStringLiteral("icon-launcher-folder-16"),
    };
    return names;
}

/** Stock backups: sibling of retired backup/icons (must not nest under it — %post wipes that tree). */
QString folderBackupRoot()
{
    return IconPaths::muotoShare() + QStringLiteral("/backup/folder-icons");
}

QString folderBackupDir(const QString& zSize)
{
    return folderBackupRoot() + QLatin1Char('/') + zSize + QLatin1Char('/');
}

QString packJollaIconPath(const QString& packRoot, const QString& zSize, const QString& iconName)
{
    const QString jollaRoot = IconPaths::resolvePackCapabilityDir(packRoot, QStringLiteral("jolla"));
    if(jollaRoot.isEmpty())
        return QString();
    const QString path = jollaRoot + QLatin1Char('/') + zSize + QStringLiteral("/icons/")
                         + iconName + QStringLiteral(".png");
    return QFileInfo::exists(path) ? path : QString();
}

bool ensureParentDir(const QString& filePath)
{
    return QDir().mkpath(QFileInfo(filePath).absolutePath());
}

bool readAll(const QString& path, QByteArray* out)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        return false;
    *out = file.readAll();
    return !out->isEmpty();
}

// Never removes the destination before the replacement bytes are in hand: the
// old remove-then-copy left the folder glyph missing if the copy failed, which
// is the damage the repair service exists to heal.
bool copyFileOverwrite(const QString& src, const QString& dst)
{
    QByteArray content;
    if(!readAll(src, &content))
        return false;

    if(QFileInfo::exists(dst))
        return FileWrite::inPlace(dst, content);

    // No live file to preserve identity from (fresh slot): a plain copy is the
    // only option, and the result being defaultuser-owned is unavoidable.
    if(!ensureParentDir(dst))
        return false;
    return QFile::copy(src, dst) && QFileInfo(dst).size() > 0;
}

bool backupOnce(const QString& livePath, const QString& backupPath)
{
    if(QFileInfo::exists(backupPath))
        return true;
    if(!QFileInfo::exists(livePath))
        return false;
    return copyFileOverwrite(livePath, backupPath);
}

/** Overlay-only folder glyph: scale the overlay frame to stock size, no inner icon. */
bool writeOverlayOnly(const QString& sizeRefPath, const QString& overlayBasePath,
                      const QString& livePath)
{
    QImage overlayBase(overlayBasePath);
    if(overlayBase.isNull())
        return false;

    int px = 0;
    {
        const QImage sizeRef(sizeRefPath);
        if(!sizeRef.isNull())
            px = qMax(sizeRef.width(), sizeRef.height());
    }
    if(px <= 0)
        px = qMax(overlayBase.width(), overlayBase.height());

    const QSize outer(px, px);
    // Empty inner icon — folders get the overlay frame alone, not stock jolla glyphs.
    const QImage out = OverlayRender::composite(overlayBase, QImage(), outer, QSize());
    if(out.isNull())
        return false;
    if(!ensureParentDir(livePath))
        return false;

    const QString tmp = livePath + QStringLiteral(".muoto-write.png");
    QFile::remove(tmp);
    if(!out.save(tmp, "PNG") || QFileInfo(tmp).size() <= 0)
    {
        QFile::remove(tmp);
        return false;
    }

    QByteArray content;
    const bool read = readAll(tmp, &content);
    QFile::remove(tmp);
    if(!read)
        return false;

    if(QFileInfo::exists(livePath))
        return FileWrite::inPlace(livePath, content);

    QFile fresh(livePath);
    if(!fresh.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return fresh.write(content) == content.size();
}

void removeBackupTrees()
{
    const QString root = folderBackupRoot();
    if(QDir(root).exists())
        QDir(root).removeRecursively();

    // Legacy nest under retired backup/icons (wiped by %post; drop if still present).
    const QString legacy = IconPaths::backupIconsRoot() + QStringLiteral("/folder-icons");
    if(QDir(legacy).exists())
        QDir(legacy).removeRecursively();
}

} // namespace

void apply(const QString& packShortName, bool overlayEnabled)
{
    if(packShortName.isEmpty() || packShortName == QLatin1String("default"))
        return;

    const QString packRoot = IconPaths::packDir(packShortName);
    if(!QDir(packRoot).exists())
        return;

    int updated = 0;
    int skipped = 0;
    for(const QString& zSize : IconPaths::jollaSizes())
    {
        const QString liveDir = IconPaths::liveJollaIconsDir(zSize);
        if(!QDir(liveDir).exists())
            continue;

        for(const QString& iconName : folderIconNames())
        {
            const QString livePath = liveDir + iconName + QStringLiteral(".png");
            const QString backupPath = folderBackupDir(zSize) + iconName + QStringLiteral(".png");
            const QString packPath = packJollaIconPath(packRoot, zSize, iconName);

            // Theming a slot whose stock we could not capture is irreversible:
            // restore() would have nothing to put back. Skip it instead.
            if(!packPath.isEmpty())
            {
                if(!backupOnce(livePath, backupPath))
                {
                    qWarning() << "FolderAmbient: no stock backup for" << livePath
                               << "- leaving it alone";
                    ++skipped;
                    continue;
                }
                if(copyFileOverwrite(packPath, livePath))
                    ++updated;
                continue;
            }

            if(overlayEnabled)
            {
                const QString overlayBase = OverlayRender::overlayBaseForDesktop(packRoot, iconName);
                if(!overlayBase.isEmpty())
                {
                    if(!backupOnce(livePath, backupPath))
                    {
                        qWarning() << "FolderAmbient: no stock backup for" << livePath
                                   << "- leaving it alone";
                        ++skipped;
                        continue;
                    }
                    if(writeOverlayOnly(backupPath, overlayBase, livePath))
                        ++updated;
                    continue;
                }
            }

            // Pack has no folder icon and overlay off (or no overlay assets): restore stock if we
            // previously themed this slot.
            if(QFileInfo::exists(backupPath) && copyFileOverwrite(backupPath, livePath))
                ++updated;
        }
    }

    qInfo() << "FolderAmbient: apply pack=" << packShortName
            << "overlay=" << overlayEnabled << "updated=" << updated
            << "skipped=" << skipped;
}

void restore()
{
    int restored = 0;
    int failures = 0;
    int pending = 0;

    for(const QString& zSize : IconPaths::jollaSizes())
    {
        const QString bakDir = folderBackupDir(zSize);
        if(!QDir(bakDir).exists())
            continue;

        const QString liveDir = IconPaths::liveJollaIconsDir(zSize);
        for(const QString& iconName : folderIconNames())
        {
            const QString backupPath = bakDir + iconName + QStringLiteral(".png");
            if(!QFileInfo::exists(backupPath))
                continue;
            ++pending;
            const QString livePath = liveDir + iconName + QStringLiteral(".png");
            if(copyFileOverwrite(backupPath, livePath))
                ++restored;
            else
                ++failures;
        }
    }

    // Only drop backups when every pending restore succeeded — otherwise keep them for retry.
    if(failures == 0)
        removeBackupTrees();
    else
        qWarning() << "FolderAmbient: restore partial failures=" << failures
                    << "restored=" << restored << "; keeping backup/folder-icons";

    qInfo() << "FolderAmbient: restore count=" << restored << "pending=" << pending
            << "failures=" << failures;
}

} // namespace FolderAmbient
