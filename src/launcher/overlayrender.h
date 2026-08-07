#ifndef OVERLAYRENDER_H
#define OVERLAYRENDER_H

#include <QImage>
#include <QSize>
#include <QString>

namespace OverlayRender {

QImage composite(const QImage& overlayBase, const QImage& innerIcon,
                 const QSize& outerSize, const QSize& innerSize);

QString overlayBaseForDesktop(const QString& packRoot, const QString& desktopPath);

} // namespace OverlayRender

#endif // OVERLAYRENDER_H
