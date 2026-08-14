#include "fontsampleprovider.h"
#include "imageutil.h"

#include <QColor>
#include <QUrlQuery>
#include <QtGlobal>

namespace {

const QString kSailSans =
        QStringLiteral("/usr/share/fonts/sail-sans-pro/SailSansPro-Light.ttf");

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

QString ttfPath(const QString& pack, const QString& basename)
{
    if(pack.isEmpty() || pack == QLatin1String("default"))
        return kSailSans;
    if(basename.isEmpty())
        return QString();
    return QStringLiteral("/usr/share/%1/font/%2.ttf").arg(pack, basename);
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

    const QStringList parts = key.split(QLatin1Char('/'), QString::SkipEmptyParts);
    if(parts.size() < 2)
        return QImage();

    const QString kind = parts.at(0);
    const QString pack = parts.at(1);
    const QString basename = parts.size() >= 3 ? parts.at(2) : QString();
    const QString path = ttfPath(pack, basename);
    if(path.isEmpty())
        return QImage();

    QImage img;
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
        if(requestedSize.width() > 0)
            width = requestedSize.width();
        const int headingPx = qMax(18, width / 12);
        const int bodyPx = qMax(14, width / 20);
        img = ImageUtil::previewTtfText(
                    path,
                    QStringLiteral("Lorem ipsum"),
                    QStringLiteral("Dolor sit amet, consectetur adipiscing elit. "
                                   "Maecenas imperdiet finibus venenatis. "
                                   "Suspendisse mollis urna sed luctus sodales."),
                    width, headingPx, bodyPx, color);
    }

    if(size)
        *size = img.size();
    return img;
}
