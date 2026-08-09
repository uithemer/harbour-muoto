#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <QImage>
#include <QSize>
#include <QString>
#include <QStringList>

// Qt-based image helpers for ConfirmPage / Cover pack previews.
namespace ImageUtil
{
    // Build a 3xN tiled montage from up to 9 PNG paths.
    // cell is the per-tile size; pad is per-side padding around each tile.
    QImage montage9(const QStringList& pngs,
                    const QSize& cell = QSize(128, 128),
                    int pad = 15);

    // Picks up to 9 random PNGs from any of {pack/native, pack/apk, pack/overlay, pack/jolla}
    // (recursive), including packs that only ship jolla/ ambient artwork.
    QStringList samplePackIcons(const QString& packDir, int count = 9);
}

#endif // IMAGEUTIL_H
