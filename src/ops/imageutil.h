#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <QImage>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QColor>

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

    // Picks up to `count` PNGs without listing whole trees. Prefers native/
    // (skips jolla/ if native had any), then apk/, then overlay/.
    QStringList samplePackIcons(const QString& packDir, int count = 8);

    // First existing silica z*/icons/icon-launcher-*.png (stop after count).
    QStringList sampleStockLauncherIcons(int count = 8);

    // Rasterize heading+body with QRawFont (does not register the TTF in
    // QFontDatabase, so Theme.fontFamily is unchanged).
    QImage previewTtfText(const QString& ttfPath,
                          const QString& heading,
                          const QString& body,
                          int width,
                          int headingPx,
                          int bodyPx,
                          const QColor& color);
}

#endif // IMAGEUTIL_H
