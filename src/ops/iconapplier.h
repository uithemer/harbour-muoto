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
    int nativeMatchCount(const QString& packName) const;
    int apkMatchCount(const QString& packName) const;

    void applyIcons(const QString& packName, bool overlay);
    void restoreIcons();
    void refreshOriginals();
    void buildPreview(const QString& packName);

signals:
    void progress(int done, int total);
    void applied();
    void restored();
    void originalsRefreshed();
    void previewReady(const QString& packName, bool ok);

private:
    QString packDir(const QString& packName) const;
};

#endif // ICONAPPLIER_H
