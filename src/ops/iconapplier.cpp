#include "iconapplier.h"
#include "iconpaths.h"
#include "iconpreviewcache.h"
#include "imageutil.h"

IconApplier::IconApplier(QObject* parent)
    : QObject(parent)
{
}

void IconApplier::buildPreview(const QString& packName)
{
    if(packName.isEmpty())
    {
        IconPreviewCache::instance().put(packName, QImage());
        emit previewReady(packName, false);
        return;
    }

    const QStringList sample = (packName == QLatin1String("default"))
            ? ImageUtil::sampleStockLauncherIcons(8)
            : ImageUtil::samplePackIcons(IconPaths::packDir(packName), 8);
    QImage img = ImageUtil::montage(sample, 4, 2);
    const bool ok = !img.isNull();

    IconPreviewCache::instance().put(packName, ok ? img : QImage());
    emit previewReady(packName, ok);
}
