#include "iconapplier.h"
#include "iconpaths.h"
#include "iconpipeline.h"
#include "iconpreviewcache.h"
#include "imageutil.h"

IconApplier::IconApplier(QObject* parent)
    : QObject(parent)
{
}

QString IconApplier::packDir(const QString& packName) const
{
    return IconPaths::packDir(packName);
}

void IconApplier::applyIcons(const QString& packName, bool runPack, bool overlay)
{
    if(packName.isEmpty())
    {
        emit applied(true, QString());
        return;
    }

    emit progress(0, 4);
    IconPipeline pipeline;
    const IconApplyResult result = pipeline.apply(packName, runPack, overlay);
    emit progress(4, 4);
    emit applied(result.ok, result.message);
}

void IconApplier::restoreIcons()
{
    emit progress(0, 1);
    IconPipeline pipeline;
    pipeline.restore();
    emit progress(1, 1);
    emit restored(true, QString());
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
