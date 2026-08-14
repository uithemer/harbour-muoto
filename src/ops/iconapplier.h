#ifndef ICONAPPLIER_H
#define ICONAPPLIER_H

#include <QColor>
#include <QObject>
#include <QString>

class IconApplier : public QObject
{
    Q_OBJECT

public:
    explicit IconApplier(QObject* parent = 0);

public slots:
    void buildPreview(const QString& packName);
    void buildStockFontPreview(int width, int headingPx, int bodyPx,
                               const QColor& color);

signals:
    void previewReady(const QString& packName, bool ok);
    void stockFontPreviewReady(bool ok);
};

#endif // ICONAPPLIER_H
