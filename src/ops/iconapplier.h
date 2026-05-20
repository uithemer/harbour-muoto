#ifndef ICONAPPLIER_H
#define ICONAPPLIER_H

#include <QHash>
#include <QImage>
#include <QObject>
#include <QString>
#include <QStringList>

class IconManifest;

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
    QString manifestPath() const;
    QString packDir(const QString& packName) const;
    QString packShortName(const QString& packName) const;
    QString hicolorAppsDir(const QString& packShort, const QString& size) const;

    QString findNativeIcon(const QString& packName, const QString& iconValue) const;
    QString findNativeIconForDesktop(const QString& packName,
                                     const QString& iconValue,
                                     const QString& desktopPath) const;
    QString findApkIcon(const QString& packName, const QString& base) const;

    QString nativeIconKey(const QString& iconValue, const QString& desktopPath) const;
    QString themedIconId(const QString& packName, const QString& iconKey) const;
    bool hicolorHasIcon(const QString& packShort, const QString& iconKey) const;

    QImage makeOverlayImage(const QString& packName, const QString& base,
                            const QString& kind, const QString& sourceIcon) const;
    QString resolveSourceIcon(const QString& iconValue, const QString& kind) const;

    QStringList nativeDesktops() const;
    QStringList apkDesktops() const;
    QString baseForNative(const QString& desktopPath) const;
    QString baseForApk(const QString& iconValue) const;

    QString nativeAppsSourceDir(const QString& packName, const QString& size) const;
    bool publishPngToAllHicolorSizes(const QString& packShort, const QString& iconKey,
                                     const QImage& image) const;
    bool publishApkKeyToHicolor(const QString& packName, const QString& base) const;

    bool installPackHicolorBridge(const QString& packName);
    void removePackHicolorBridge(const QString& packName);
    void publishOverlayIconsToHicolor(const QString& packName);

    void chownToDefaultUser(const QString& path) const;
};

#endif // ICONAPPLIER_H
