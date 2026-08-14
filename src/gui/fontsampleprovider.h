#ifndef FONTSAMPLEPROVIDER_H
#define FONTSAMPLEPROVIDER_H

#include <QQuickImageProvider>

// Rasterizes pack (and stock Sail Sans) samples with QRawFont.
// Never calls QFontDatabase::addApplicationFont.
//
// image://muoto-font/aa/default
// image://muoto-font/aa/<pack>/<basename>
// image://muoto-font/lorem/default
// image://muoto-font/lorem/<pack>/<basename>
// Optional ?c=<hex color>
class FontSampleProvider : public QQuickImageProvider
{
public:
    FontSampleProvider();

    QImage requestImage(const QString& id,
                        QSize* size,
                        const QSize& requestedSize) override;
};

#endif // FONTSAMPLEPROVIDER_H
