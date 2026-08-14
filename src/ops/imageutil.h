#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <QImage>
#include <QSize>
#include <QString>
#include <QStringList>

// Qt-based image helpers for ConfirmPage / Cover pack previews.
namespace ImageUtil
{
    // Build a cols×rows tiled montage from PNG paths (unused cells stay empty).
    // cell is the per-tile size; pad is per-side padding around each tile.
    QImage montage(const QStringList& pngs,
                   int cols = 4,
                   int rows = 2,
                   const QSize& cell = QSize(128, 128),
                   int pad = 15);

    // Picks up to `count` random PNGs from any of {pack/native, pack/apk, pack/overlay, pack/jolla}
    // (recursive), including packs that only ship jolla/ ambient artwork.
    QStringList samplePackIcons(const QString& packDir, int count = 8);
}

#endif // IMAGEUTIL_H
