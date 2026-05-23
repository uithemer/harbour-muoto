#ifndef ICONAPPLIER_H
#define ICONAPPLIER_H

#include <QObject>
#include <QString>
#include <QStringList>

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
    QString findNativeIcon(const QString& packName, const QString& iconValue) const;
    QString findNativeIconForDesktop(const QString& packName,
                                     const QString& iconValue,
                                     const QString& desktopPath) const;
    QString findApkIcon(const QString& packName, const QString& base) const;

    QStringList nativeDesktops() const;
    QStringList apkDesktops() const;
    QString baseForNative(const QString& desktopPath) const;
    QString baseForApk(const QString& iconValue) const;
};

#endif // ICONAPPLIER_H
