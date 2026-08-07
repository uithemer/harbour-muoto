#include "svgiconrender.h"

#include <silicatheme.h>
#include <QDebug>
#include <QPainter>
#include <QSvgRenderer>

QImage renderSvgIcon(const QByteArray& data, const QSize& requestedSize)
{
    QSize size = requestedSize;

    if(!size.isValid())
    {
        const int defaultSize = qRound(Silica::Theme::instance()->iconSizeLauncher());
        size = QSize(defaultSize, defaultSize);
    }

    QSvgRenderer renderer(data);
    if(!renderer.isValid())
    {
        qWarning() << "muoto-launcher: invalid SVG data";
        return {};
    }

    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    renderer.render(&painter);

    return image;
}
