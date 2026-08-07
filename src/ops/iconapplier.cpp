#include "iconapplier.h"
#include "iconpaths.h"
#include "iconpreviewcache.h"
#include "imageutil.h"

IconApplier::IconApplier(QObject* parent)
    : QObject(parent)
{
}

void IconApplier::applyIcons(const QString& packName, bool runPack, bool overlay)
{
    Q_UNUSED(packName);
    Q_UNUSED(runPack);
    Q_UNUSED(overlay);
    emit applied(false, QStringLiteral("use session launcher D-Bus"));
}

void IconApplier::restoreIcons()
{
    emit restored(false, QStringLiteral("use session launcher D-Bus"));
}

void IconApplier::buildPreview(const QString& packName)
{
    if(packName.isEmpty())
    {
        IconPreviewCache::instance().put(packName, QImage());
        emit previewReady(packName, false);
        return;
    }

    const QStringList sample = ImageUtil::samplePackIcons(IconPaths::packDir(packName), 9);
    QImage img = ImageUtil::montage9(sample);
    const bool ok = !img.isNull();

    IconPreviewCache::instance().put(packName, ok ? img : QImage());
    emit previewReady(packName, ok);
}
