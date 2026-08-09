#ifndef ICONAPPLIER_H
#define ICONAPPLIER_H

#include <QObject>
#include <QString>

class IconApplier : public QObject
{
    Q_OBJECT

public:
    explicit IconApplier(QObject* parent = 0);

public slots:
    void buildPreview(const QString& packName);

signals:
    void previewReady(const QString& packName, bool ok);
};

#endif // ICONAPPLIER_H
