#ifndef ICONPREVIEWPROVIDER_H
#define ICONPREVIEWPROVIDER_H

#include <QQuickImageProvider>

// Serves the icon-pack preview QImage cached in IconPreviewCache through QML
// image:// URLs of the form "image://uithemer/preview/<packName>?t=<ts>".
//
// The "?t=..." query is treated as an opaque cache buster: QML strips it
// before reaching us, but we also drop everything from the first '?' just in
// case. The optional leading "preview/" segment is also stripped so consumers
// can pick any URL hierarchy.
//
// Returns an empty QImage when nothing is cached for that pack; QML treats
// that as Image.Error which the dialog uses to flip to a "No preview
// available" fallback label.
class IconPreviewProvider : public QQuickImageProvider
{
public:
    IconPreviewProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    QImage requestImage(const QString& id,
                        QSize* size,
                        const QSize& requestedSize) override;
};

#endif // ICONPREVIEWPROVIDER_H
