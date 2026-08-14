#include "fontsampleprovider.h"
#include "imageutil.h"

#include <QColor>
#include <QHash>
#include <QUrlQuery>
#include <QtGlobal>

namespace {

const QString kSailSans =
        QStringLiteral("/usr/share/fonts/sail-sans-pro/SailSansPro-Light.ttf");

const int kImageCacheCap = 32;

QHash<QString, QImage> g_imageCache;
QStringList g_imageCacheOrder;

QString stripQuery(const QString& id, QString* query)
{
    const int q = id.indexOf(QLatin1Char('?'));
    if(q < 0)
    {
        if(query)
            query->clear();
        return id;
    }
    if(query)
        *query = id.mid(q + 1);
    return id.left(q);
}

QColor colorFromQuery(const QString& query, const QColor& fallback)
{
    if(query.isEmpty())
        return fallback;

    QUrlQuery uq(query);
    QString c = uq.queryItemValue(QStringLiteral("c"));
    if(c.isEmpty())
        return fallback;
    if(!c.startsWith(QLatin1Char('#')))
        c.prepend(QLatin1Char('#'));
    const QColor col(c);
    return col.isValid() ? col : fallback;
}

int intFromQuery(const QString& query, const char* name, int fallback)
{
    if(query.isEmpty())
        return fallback;
    QUrlQuery uq(query);
    const QString v = uq.queryItemValue(QString::fromLatin1(name));
    if(v.isEmpty())
        return fallback;
    bool ok = false;
    const int n = v.toInt(&ok);
    return (ok && n > 0) ? n : fallback;
}

QString ttfPath(const QString& pack, const QString& basename)
{
    if(pack.isEmpty() || pack == QLatin1String("default"))
        return kSailSans;
    if(basename.isEmpty())
        return QString();
    return QStringLiteral("/usr/share/%1/font/%2.ttf").arg(pack, basename);
}

QImage cachedImage(const QString& cacheKey)
{
    const auto it = g_imageCache.constFind(cacheKey);
    if(it == g_imageCache.constEnd())
        return QImage();
    return it.value();
}

void storeImage(const QString& cacheKey, const QImage& img)
{
    if(img.isNull() || cacheKey.isEmpty())
        return;
    if(g_imageCache.contains(cacheKey))
        return;
    while(g_imageCache.size() >= kImageCacheCap && !g_imageCacheOrder.isEmpty())
    {
        const QString oldest = g_imageCacheOrder.takeFirst();
        g_imageCache.remove(oldest);
    }
    g_imageCache.insert(cacheKey, img);
    g_imageCacheOrder.append(cacheKey);
}

} // namespace

FontSampleProvider::FontSampleProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage FontSampleProvider::requestImage(const QString& id,
                                        QSize* size,
                                        const QSize& requestedSize)
{
    QString query;
    const QString key = stripQuery(id, &query);
    const QColor color = colorFromQuery(query, QColor(Qt::white));

    const QString cacheKey = key
            + QLatin1Char('|') + color.name()
            + QLatin1Char('|') + QString::number(requestedSize.width())
            + QLatin1Char('x') + QString::number(requestedSize.height())
            + QLatin1Char('|') + query;
    QImage img = cachedImage(cacheKey);
    if(!img.isNull())
    {
        if(size)
            *size = img.size();
        return img;
    }

    const QStringList parts = key.split(QLatin1Char('/'), QString::SkipEmptyParts);
    if(parts.size() < 2)
        return QImage();

    const QString kind = parts.at(0);
    const QString pack = parts.at(1);
    const QString basename = parts.size() >= 3 ? parts.at(2) : QString();
    const QString path = ttfPath(pack, basename);
    if(path.isEmpty())
        return QImage();

    if(kind == QLatin1String("aa"))
    {
        int px = 48;
        if(requestedSize.height() > 0)
            px = requestedSize.height();
        else if(requestedSize.width() > 0)
            px = requestedSize.width();
        img = ImageUtil::previewTtfGlyphs(path, QStringLiteral("Aa"), px, color);
    }
    else if(kind == QLatin1String("lorem"))
    {
        int width = 480;
        int height = 240;
        if(requestedSize.width() > 0)
            width = requestedSize.width();
        if(requestedSize.height() > 0)
            height = requestedSize.height();
        else
            height = qMax(1, width / 2);
        const int pad = qMax(8, width / 32);
        const int usable = qMax(1, height - 2 * pad);
        // Heading + gap + ~3–4 wrapped body lines must fit in usable.
        int headingPx = intFromQuery(query, "h", qMax(24, usable / 6));
        int bodyPx = intFromQuery(query, "b", qMax(16, headingPx * 2 / 3));
        headingPx = qMin(headingPx, qMax(8, usable / 6));
        bodyPx = qMin(bodyPx, qMax(8, usable / 10));
        img = ImageUtil::previewTtfText(
                    path,
                    QStringLiteral("Lorem ipsum"),
                    QStringLiteral("Dolor sit amet, consectetur adipiscing elit. "
                                   "Maecenas imperdiet finibus venenatis."),
                    width, headingPx, bodyPx, color, height, pad);
    }

    storeImage(cacheKey, img);
    if(size)
        *size = img.size();
    return img;
}
