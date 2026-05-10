#include "imageutil.h"

#include <QDir>
#include <QDirIterator>
#include <QPainter>
#include <QFileInfo>
#include <QtGlobal>
#include <algorithm>
#include <random>

namespace ImageUtil
{

QImage composite(const QImage& overlayBase, const QImage& innerIcon,
                 const QSize& outerSize, const QSize& innerSize)
{
    QImage out(outerSize, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);

    if(!overlayBase.isNull())
    {
        const QImage scaled = overlayBase.scaled(outerSize,
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::SmoothTransformation);
        p.drawImage(QPoint(0, 0), scaled);
    }

    if(!innerIcon.isNull())
    {
        const QImage scaledInner = innerIcon.scaled(innerSize,
                                                    Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation);
        const int x = (outerSize.width()  - scaledInner.width())  / 2;
        const int y = (outerSize.height() - scaledInner.height()) / 2;
        p.drawImage(QPoint(x, y), scaledInner);
    }

    p.end();
    return out;
}

QImage montage9(const QStringList& pngs, const QSize& cell, int pad)
{
    const int cols = 3;
    const int n = pngs.size();
    if(n <= 0)
        return QImage();

    const int rows = (n + cols - 1) / cols;
    const int tileW = cell.width() + pad * 2;
    const int tileH = cell.height() + pad * 2;

    QImage out(tileW * cols, tileH * rows, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    for(int i = 0; i < n; ++i)
    {
        QImage src(pngs[i]);
        if(src.isNull())
            continue;

        const int r = i / cols;
        const int c = i % cols;

        const QImage scaled = src.scaled(cell,
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);

        const int x = c * tileW + pad + (cell.width()  - scaled.width())  / 2;
        const int y = r * tileH + pad + (cell.height() - scaled.height()) / 2;
        p.drawImage(QPoint(x, y), scaled);
    }

    p.end();
    return out;
}

static QStringList collectPngs(const QString& root)
{
    QStringList all;
    if(!QFileInfo::exists(root))
        return all;

    QDirIterator it(root, QStringList() << QStringLiteral("*.png"),
                    QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
        all << it.next();
    return all;
}

QStringList samplePackIcons(const QString& packDir, int count)
{
    QStringList all;
    all << collectPngs(packDir + QStringLiteral("/native"));
    all << collectPngs(packDir + QStringLiteral("/apk"));
    all << collectPngs(packDir + QStringLiteral("/overlay"));
    all << collectPngs(packDir + QStringLiteral("/jolla"));

    if(all.isEmpty())
        return all;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(all.begin(), all.end(), gen);

    if(all.size() > count)
        all = all.mid(0, count);

    return all;
}

QString randomOverlayBase(const QString& packDir)
{
    const QStringList all = collectPngs(packDir + QStringLiteral("/overlay"));
    if(all.isEmpty())
        return QString();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, all.size() - 1);
    return all[dist(gen)];
}

} // namespace ImageUtil
