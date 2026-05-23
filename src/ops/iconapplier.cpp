#include "iconapplier.h"
#include "iconpaths.h"
#include "iconpipeline.h"
#include "iconpreviewcache.h"
#include "imageutil.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

IconApplier::IconApplier(QObject* parent)
    : QObject(parent)
{
}

QString IconApplier::packDir(const QString& packName) const
{
    return IconPaths::packDir(packName);
}

int IconApplier::nativeMatchCount(const QString& packName) const
{
    if(packName.isEmpty())
        return 0;

    const QSet<QString> live = IconPaths::liveHicolorAppKeys();
    const QSet<QString> pack = IconPaths::packIconKeys(packName);
    int count = 0;
    for(const QString& k : live)
    {
        if(pack.contains(k))
            ++count;
    }
    return count;
}

int IconApplier::apkMatchCount(const QString& packName) const
{
    if(packName.isEmpty())
        return 0;

    const QSet<QString> pack = IconPaths::packApkKeys(packName);
    QDir apk(IconPaths::liveApkLauncherDir());
    if(!apk.exists())
        return 0;

    int count = 0;
    const QStringList pngs = apk.entryList(QStringList() << QStringLiteral("*.png"),
                                           QDir::Files);
    for(const QString& f : pngs)
    {
        if(pack.contains(QFileInfo(f).completeBaseName()))
            ++count;
    }
    return count;
}

void IconApplier::applyIcons(const QString& packName, bool runPack, bool overlay)
{
    if(packName.isEmpty())
    {
        emit applied();
        return;
    }

    emit progress(0, 4);
    IconPipeline pipeline;
    pipeline.apply(packName, runPack, overlay);
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
