#include "imageutil.h"
#include "iconpaths.h"

#include <QDir>
#include <QDirIterator>
#include <QGlyphRun>
#include <QPainter>
#include <QRawFont>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <random>

namespace ImageUtil
{

namespace
{
    QStringList collectPngsUnderCapability(const QString& packRoot,
                                           const QString& capability,
                                           int limit)
    {
        if(limit <= 0)
            return QStringList();

        const QString path = IconPaths::resolvePackCapabilityDir(packRoot, capability);
        if(path.isEmpty())
            return QStringList();

        QStringList all;
        QDirIterator it(path, QStringList() << QStringLiteral("*.png"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
        {
            all << it.next();
            if(all.size() >= limit)
                break;
        }
        return all;
    }
}

QImage montage(const QStringList& pngs, int cols, int rows, const QSize& cell, int pad)
{
    if(cols <= 0 || rows <= 0 || pngs.isEmpty())
        return QImage();

    const int n = qMin(pngs.size(), cols * rows);
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

QStringList samplePackIcons(const QString& packDir, int count)
{
    if(count <= 0)
        return QStringList();

    const QString packRoot = IconPaths::packDir(packDir);

    const QStringList native = collectPngsUnderCapability(
                packRoot, QStringLiteral("native"), count);
    const QStringList jolla = native.isEmpty()
            ? collectPngsUnderCapability(packRoot, QStringLiteral("jolla"), count)
            : QStringList();
    const QStringList primary = native.isEmpty() ? jolla : native;

    const QStringList apk = collectPngsUnderCapability(
                packRoot, QStringLiteral("apk"), count);

    QStringList all;
    const int apkTake = apk.isEmpty() ? 0 : qMin(apk.size(), count > 1 ? 2 : count);
    const int primaryTake = qMin(primary.size(), count - apkTake);
    all << primary.mid(0, primaryTake);
    all << apk.mid(0, qMin(apk.size(), count - all.size()));

    const int remain = count - all.size();
    if(remain > 0)
        all << collectPngsUnderCapability(packRoot, QStringLiteral("overlay"), remain);

    if(all.isEmpty())
        return all;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(all.begin(), all.end(), gen);

    if(all.size() > count)
        all = all.mid(0, count);

    return all;
}

QStringList sampleStockLauncherIcons(int count)
{
    if(count <= 0)
        return QStringList();

    const QStringList& sizes = IconPaths::jollaSizes();
    for(int s = 0; s < sizes.size(); ++s)
    {
        const QString dir = IconPaths::liveJollaIconsDir(sizes.at(s));
        if(!QDir(dir).exists())
            continue;

        QStringList all;
        QDirIterator it(dir, QStringList() << QStringLiteral("icon-launcher-*.png"),
                        QDir::Files);
        while(it.hasNext())
        {
            all << it.next();
            if(all.size() >= count)
                break;
        }
        if(!all.isEmpty())
            return all;
    }
    return QStringList();
}

namespace
{
qreal glyphRunWidth(const QRawFont& font, const QString& text)
{
    if(text.isEmpty())
        return 0;

    const QVector<quint32> glyphs = font.glyphIndexesForString(text);
    const QVector<QPointF> adv = font.advancesForGlyphIndexes(glyphs);
    qreal w = 0;
    for(int i = 0; i < adv.size(); ++i)
        w += adv.at(i).x();
    return w;
}

QStringList wrapText(const QRawFont& font, const QString& text, qreal maxWidth)
{
    QStringList lines;
    const QStringList paragraphs = text.split(QLatin1Char('\n'));
    for(int p = 0; p < paragraphs.size(); ++p)
    {
        const QStringList words = paragraphs.at(p).split(QLatin1Char(' '),
                                                         QString::SkipEmptyParts);
        QString line;
        for(int i = 0; i < words.size(); ++i)
        {
            const QString trial = line.isEmpty()
                    ? words.at(i)
                    : (line + QLatin1Char(' ') + words.at(i));
            if(!line.isEmpty() && glyphRunWidth(font, trial) > maxWidth)
            {
                lines << line;
                line = words.at(i);
            }
            else
            {
                line = trial;
            }
        }
        if(!line.isEmpty())
            lines << line;
        else if(paragraphs.at(p).isEmpty())
            lines << QString();
    }
    return lines;
}

qreal drawLines(QPainter* p, const QRawFont& font, const QStringList& lines,
                qreal x, qreal y, const QColor& color)
{
    const qreal lineHeight = font.ascent() + font.descent()
            + qMax(qreal(2), font.ascent() * qreal(0.2));
    p->setPen(color);
    for(int i = 0; i < lines.size(); ++i)
    {
        const QString& line = lines.at(i);
        if(!line.isEmpty())
        {
            const QVector<quint32> glyphs = font.glyphIndexesForString(line);
            const QVector<QPointF> adv = font.advancesForGlyphIndexes(glyphs);
            QVector<QPointF> positions;
            positions.reserve(glyphs.size());
            QPointF pen(x, y);
            for(int g = 0; g < glyphs.size(); ++g)
            {
                positions.append(pen);
                pen += adv.at(g);
            }
            QGlyphRun run;
            run.setRawFont(font);
            run.setGlyphIndexes(glyphs);
            run.setPositions(positions);
            p->drawGlyphRun(QPointF(0, 0), run);
        }
        y += lineHeight;
    }
    return y;
}
} // namespace

QImage previewTtfText(const QString& ttfPath,
                      const QString& heading,
                      const QString& body,
                      int width,
                      int headingPx,
                      int bodyPx,
                      const QColor& color)
{
    if(ttfPath.isEmpty() || width <= 0 || headingPx <= 0 || bodyPx <= 0)
        return QImage();

    const QRawFont headingFont(ttfPath, headingPx);
    const QRawFont bodyFont(ttfPath, bodyPx);
    if(!headingFont.isValid() || !bodyFont.isValid())
        return QImage();

    const QStringList headingLines = wrapText(headingFont, heading, width);
    const QStringList bodyLines = wrapText(bodyFont, body, width);
    const qreal headingLh = headingFont.ascent() + headingFont.descent()
            + qMax(qreal(2), headingFont.ascent() * qreal(0.2));
    const qreal bodyLh = bodyFont.ascent() + bodyFont.descent()
            + qMax(qreal(2), bodyFont.ascent() * qreal(0.2));
    const qreal gap = headingPx * 0.4;
    const int height = qMax(1, qRound(headingFont.ascent()
                                      + headingLines.size() * headingLh
                                      + gap
                                      + bodyLines.size() * bodyLh
                                      + bodyFont.descent()));

    QImage out(width, height, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.setRenderHint(QPainter::TextAntialiasing);
    qreal y = headingFont.ascent();
    y = drawLines(&p, headingFont, headingLines, 0, y, color);
    y += gap;
    drawLines(&p, bodyFont, bodyLines, 0, y, color);
    p.end();
    return out;
}

} // namespace ImageUtil
