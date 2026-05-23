#ifndef ICONPATHS_H
#define ICONPATHS_H

#include <QSet>
#include <QString>
#include <QStringList>

namespace IconPaths
{
    QString uithemerShare();
    QString backupIconsRoot();
    QString tmpDir();
    QString packDir(const QString& packName);
    QString resolvePackCapabilityDir(const QString& packRoot, const QString& capability);
    bool packCapabilityUsable(const QString& packRoot, const QString& capability);

    const QStringList& nativeHicolorSizes();
    const QStringList& jollaSizes();
    const QStringList& apkPackSizes();

    QString liveJollaIconsDir(const QString& zSize);
    QString liveNativeAppsDir(const QString& size);
    QString liveApkLauncherDir();

    QString backupJollaIconsDir(const QString& zSize);
    QString backupNativeAppsDir(const QString& size);
    QString backupApkDir();

    bool copyFileIgnoreExistingBackup(const QString& src, const QString& dst);
    bool copyFileExistingOnly(const QString& src, const QString& dst);
    int copyPngDirIgnoreExistingBackup(const QString& srcDir, const QString& dstDir);
    int copyPngDirExistingOnly(const QString& srcDir, const QString& dstDir);

    void chownApkLauncherTree();
    QString nativeAppsSourceDir(const QString& packName, const QString& size);

    QSet<QString> packIconKeys(const QString& packName);
    QSet<QString> packApkKeys(const QString& packName);
    QSet<QString> liveHicolorAppKeys();
}

#endif // ICONPATHS_H
