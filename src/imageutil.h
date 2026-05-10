#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <QImage>
#include <QSize>
#include <QString>
#include <QStringList>

// Qt-based image helpers replacing the previous ImageMagick calls
// (`convert` for overlays, `montage` for previews).
namespace ImageUtil
{
    // Composite a base "overlay" image with an inner icon centered on top.
    // outerSize is the final canvas size; innerSize is the rect the icon is fitted to.
    QImage composite(const QImage& overlayBase, const QImage& innerIcon,
                     const QSize& outerSize, const QSize& innerSize);

    // Build a 3xN tiled montage from up to 9 PNG paths.
    // cell is the per-tile size; pad is per-side padding around each tile.
    QImage montage9(const QStringList& pngs,
                    const QSize& cell = QSize(128, 128),
                    int pad = 15);

    // Picks up to 9 random PNGs from any of {pack/native, pack/apk, pack/overlay} (recursive).
    QStringList samplePackIcons(const QString& packDir, int count = 9);

    // Picks one random overlay base PNG from `<packDir>/overlay`. Returns empty path if none.
    QString randomOverlayBase(const QString& packDir);
}

#endif // IMAGEUTIL_H
