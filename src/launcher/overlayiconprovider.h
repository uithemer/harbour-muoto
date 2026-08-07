#ifndef OVERLAYICONPROVIDER_H
#define OVERLAYICONPROVIDER_H

#include "iconprovider.h"

#include <QString>

class OverlayIconProvider : public IconProvider
{
    Q_OBJECT

public:
    OverlayIconProvider(const QString& packRoot, const QString& desktopPath,
                        QObject* parent = nullptr);

    QImage requestImage(const QSize& requestedSize) override;

private:
    QString m_packRoot;
    QString m_desktopPath;
};

#endif // OVERLAYICONPROVIDER_H
