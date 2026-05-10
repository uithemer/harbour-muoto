#include "iconpreviewprovider.h"

#include <QImage>

#include "iconpreviewcache.h"

QImage IconPreviewProvider::requestImage(const QString& id,
                                         QSize* size,
                                         const QSize& requestedSize)
{
    QString key = id;

    const int q = key.indexOf(QLatin1Char('?'));
    if(q >= 0)
        key = key.left(q);

    if(key.startsWith(QStringLiteral("preview/")))
        key = key.mid(8);

    const QImage img = IconPreviewCache::instance().get(key);

    if(size)
        *size = img.size();

    if(img.isNull())
        return img;

    if(requestedSize.isValid()
            && (requestedSize.width() > 0 || requestedSize.height() > 0))
    {
        return img.scaled(requestedSize,
                          Qt::KeepAspectRatio,
                          Qt::SmoothTransformation);
    }

    return img;
}
