#include "iconapplier.h"
#include "desktopfile.h"
#include "iconpaths.h"
#include "iconpipeline.h"
#include "iconpreviewcache.h"
#include "imageutil.h"

#include <QDir>
#include <QFileInfo>

namespace
{
    const char* kNativeAppsDir = "/usr/share/applications";
    const char* kApkAppsDir = "/home/defaultuser/.local/share/applications";

    QString iconKeyFromValue(const QString& iconValue)
    {
        QString key = iconValue;
        if(key.contains(QLatin1Char('/')))
            key = QFileInfo(key).completeBaseName();
        if(key.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
            key.chop(4);
        return key;
    }
}

IconApplier::IconApplier(QObject* parent)
    : QObject(parent)
{
}

QString IconApplier::packDir(const QString& packName) const
{
    return IconPaths::packDir(packName);
}

QString IconApplier::findNativeIcon(const QString& packName, const QString& iconValue) const
{
    const QString key = iconKeyFromValue(iconValue);
    if(key.isEmpty())
        return QString();

    const QString root = packDir(packName);

    for(const QString& s : IconPaths::nativeHicolorSizes())
    {
        const QString p = root + QStringLiteral("/native/") + s
                          + QStringLiteral("/apps/") + key + QStringLiteral(".png");
        if(QFileInfo::exists(p))
            return p;
    }

    for(const QString& s : IconPaths::jollaSizes())
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
    for(const QString& s : IconPaths::apkPackSizes())
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
            for(const QString& s : IconPaths::apkPackSizes())
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

void IconApplier::applyIcons(const QString& packName, bool overlay)
{
    if(packName.isEmpty())
    {
        emit applied();
        return;
    }

    emit progress(0, 4);
    IconPipeline pipeline;
    pipeline.apply(packName, overlay);
    emit progress(4, 4);
    emit applied();
}

void IconApplier::restoreIcons()
{
    emit progress(0, 1);
    IconPipeline pipeline;
    pipeline.restore();
    emit progress(1, 1);
    emit restored();
}

void IconApplier::refreshOriginals()
{
    emit originalsRefreshed();
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
