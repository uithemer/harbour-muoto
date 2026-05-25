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

    // Maps silica z tier to hicolor apps size; empty for unmapped tiers (z1.5-large).
    QString hicolorSizeForJollaZ(const QString& zSize);
    bool isJollaLauncherIconKey(const QString& baseName);

    // Read-only stock Jolla icons under /usr/share/themes/.../silica (never written).
    QString stockJollaIconsSourceDir(const QString& zSize);
    QString liveNativeAppsDir(const QString& size);
    QString liveApkLauncherDir();
    QString liveApkCustomDir();
    QString liveApkApplicationsDir();

    QString apkLauncherIconSegment();
    QString apkCustomSegment();

    QString backupNativeAppsDir(const QString& size);
    QString backupApkDir();

    bool copyFileIgnoreExistingBackup(const QString& src, const QString& dst);
    bool copyFileExistingOnly(const QString& src, const QString& dst);
    int copyPngDirIgnoreExistingBackup(const QString& srcDir, const QString& dstDir);
    int copyPngDirExistingOnly(const QString& srcDir, const QString& dstDir);
    int copyApkPackPngsToCustomDir(const QString& packSrcDir);

    QString packApkPngPath(const QString& packName, const QString& fileName);

    void chownApkLauncherTree();
    void chownToDefaultUser(const QString& path);
    QString nativeAppsSourceDir(const QString& packName, const QString& size);

    QSet<QString> packIconKeys(const QString& packName);
    QSet<QString> packApkKeys(const QString& packName);
    QSet<QString> liveHicolorAppKeys();
}

#endif // ICONPATHS_H
