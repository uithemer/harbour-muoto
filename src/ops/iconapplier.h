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
    void applyIcons(const QString& packName, bool runPack, bool overlay);
    void restoreIcons();
    void buildPreview(const QString& packName);

signals:
    void progress(int done, int total);
    void applied(bool ok, const QString& message);
    void restored(bool ok, const QString& message);
    void previewReady(const QString& packName, bool ok);

private:
    QString packDir(const QString& packName) const;
};

#endif // ICONAPPLIER_H
