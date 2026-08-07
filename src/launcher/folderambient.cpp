#include "folderambient.h"

#include "iconpaths.h"
#include "launcherpaths.h"
#include "overlayrender.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QRegularExpression>
#include <QTextStream>

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

QString folderBackupDir(const QString& zSize)
{
    return IconPaths::backupIconsRoot() + QStringLiteral("/folder-icons/") + zSize
           + QLatin1Char('/');
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

bool copyFileOverwrite(const QString& src, const QString& dst)
{
    if(!QFileInfo::exists(src))
        return false;
    if(!ensureParentDir(dst))
        return false;
    if(QFileInfo::exists(dst) && !QFile::remove(dst))
        return false;
    return QFile::copy(src, dst);
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
    if(QFileInfo::exists(tmp))
        QFile::remove(tmp);
    if(!out.save(tmp, "PNG"))
    {
        QFile::remove(tmp);
        return false;
    }
    if(QFileInfo::exists(livePath) && !QFile::remove(livePath))
    {
        QFile::remove(tmp);
        return false;
    }
    if(!QFile::rename(tmp, livePath))
    {
        QFile::remove(tmp);
        return false;
    }
    return true;
}

} // namespace

void normalizeDirectoryRedirects()
{
    const QString dirPath = LauncherPaths::lipstickConfigDir();
    if(dirPath.isEmpty())
        return;

    QDir dir(dirPath);
    if(!dir.exists())
        return;

    const QRegularExpression folderRe(QStringLiteral("^Folder(\\d+)\\.directory$"));
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("Folder*.directory"),
                                            QDir::Files);
    const QString genRoot = LauncherPaths::generatedIconsDir();

    for(const QString& name : files)
    {
        const QString path = dir.absoluteFilePath(name);
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        QString content = QString::fromUtf8(f.readAll());
        f.close();

        QString iconValue;
        QTextStream in(&content, QIODevice::ReadOnly);
        QString line;
        while(in.readLineInto(&line))
        {
            if(line.startsWith(QLatin1String("Icon=")))
            {
                iconValue = line.mid(5).trimmed();
                break;
            }
        }

        if(iconValue.isEmpty())
            continue;
        if(!iconValue.startsWith(genRoot) && !iconValue.contains(QStringLiteral("/launcher-icons/")))
            continue;

        const QRegularExpressionMatch m = folderRe.match(name);
        const QString stock = m.hasMatch()
            ? QStringLiteral("icon-launcher-folder-%1").arg(m.captured(1), 2, QLatin1Char('0'))
            : QStringLiteral("icon-launcher-folder-01");
        QString out;
        QTextStream writer(&out);
        QTextStream reader(&content, QIODevice::ReadOnly);
        bool wroteIcon = false;
        while(reader.readLineInto(&line))
        {
            if(line.startsWith(QLatin1String("Icon=")))
            {
                writer << QStringLiteral("Icon=") << stock << QLatin1Char('\n');
                wroteIcon = true;
            }
            else
            {
                writer << line << QLatin1Char('\n');
            }
        }
        if(!wroteIcon)
            writer << QStringLiteral("Icon=") << stock << QLatin1Char('\n');

        if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            continue;
        f.write(out.toUtf8());
        f.close();
        qDebug() << "FolderAmbient: normalized" << path << "->" << stock;
    }
}

void apply(const QString& packShortName, bool overlayEnabled)
{
    normalizeDirectoryRedirects();

    if(packShortName.isEmpty() || packShortName == QLatin1String("default"))
        return;

    const QString packRoot = IconPaths::packDir(packShortName);
    if(!QDir(packRoot).exists())
        return;

    int updated = 0;
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

            if(!packPath.isEmpty())
            {
                backupOnce(livePath, backupPath);
                if(copyFileOverwrite(packPath, livePath))
                    ++updated;
                continue;
            }

            if(overlayEnabled)
            {
                const QString overlayBase = OverlayRender::overlayBaseForDesktop(packRoot, iconName);
                if(!overlayBase.isEmpty())
                {
                    backupOnce(livePath, backupPath);
                    const QString sizeRef = QFileInfo::exists(backupPath) ? backupPath : livePath;
                    if(writeOverlayOnly(sizeRef, overlayBase, livePath))
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

    qDebug() << "FolderAmbient: apply pack=" << packShortName
             << "overlay=" << overlayEnabled << "updated=" << updated;
}

void restore()
{
    normalizeDirectoryRedirects();

    int restored = 0;
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
            const QString livePath = liveDir + iconName + QStringLiteral(".png");
            if(copyFileOverwrite(backupPath, livePath))
                ++restored;
        }
    }

    const QString root = IconPaths::backupIconsRoot() + QStringLiteral("/folder-icons");
    if(QDir(root).exists())
        QDir(root).removeRecursively();

    qDebug() << "FolderAmbient: restore count=" << restored;
}

} // namespace FolderAmbient
