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

void IconApplier::buildStockFontPreview(int width, int headingPx, int bodyPx,
                                        const QColor& color)
{
    static const QString kKey = QStringLiteral("stock-font");
    static const QString kTtf =
            QStringLiteral("/usr/share/fonts/sail-sans-pro/SailSansPro-Light.ttf");

    const QImage img = ImageUtil::previewTtfText(
                kTtf,
                QStringLiteral("Lorem ipsum"),
                QStringLiteral("Dolor sit amet, consectetur adipiscing elit. "
                               "Maecenas imperdiet finibus venenatis. "
                               "Suspendisse mollis urna sed luctus sodales."),
                width, headingPx, bodyPx, color);
    const bool ok = !img.isNull();
    IconPreviewCache::instance().put(kKey, ok ? img : QImage());
    emit stockFontPreviewReady(ok);
}
