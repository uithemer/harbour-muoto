#include "iconpaths.h"

#include <QDir>
#include <QFileInfo>

#include <pwd.h>
#include <unistd.h>

namespace
{
    const char *kMuotoShare = "/usr/share/harbour-muoto";
    const char *kPackPrefix = "/usr/share/harbour-themepack-";
    const char *kJollaRoot = "/usr/share/themes/sailfish-default/silica";

    const QStringList kJolla = {
        QStringLiteral("z2.0"),
        QStringLiteral("z1.75"),
        QStringLiteral("z1.5-large"),
        QStringLiteral("z1.5"),
        QStringLiteral("z1.25"),
        QStringLiteral("z1.0"),
    };

    QString packShortName(QString packName)
    {
        static const QString kBare = QStringLiteral("harbour-themepack-");
        const QString kFullPrefix = QString::fromLatin1(kPackPrefix);
        if (packName.startsWith(kFullPrefix))
            packName = packName.mid(kFullPrefix.size());
        else if (packName.startsWith(kBare))
            packName = packName.mid(kBare.size());
        return packName;
    }

    QString userHomeDir()
    {
        struct passwd *pw = getpwuid(getuid());
        return pw ? QString::fromUtf8(pw->pw_dir) : QString();
    }

    QString remapNemoHome(const QString &path)
    {
        static const QString kNemoHome = QStringLiteral("/home/nemo");
        const QString home = userHomeDir();
        if (home.isEmpty() || home == kNemoHome)
            return path;
        if (path.startsWith(kNemoHome))
            return home + path.mid(kNemoHome.size());
        return path;
    }

    QString packHomeRoot(const QString &packRoot)
    {
        const QString home = userHomeDir();
        const QString base = QFileInfo(packRoot).fileName();
        if (home.isEmpty() || base.isEmpty())
            return QString();
        return home + QStringLiteral("/.themepack/") + base;
    }

    QString existingCapabilityDir(const QString &path)
    {
        const QFileInfo fi(path);
        if (fi.isDir() && !fi.isSymLink() && fi.exists())
            return fi.absoluteFilePath();
        return QString();
    }

    QString resolveCapabilityDir(const QString &packRoot, const QString &capability)
    {
        const QString entry = packRoot + QLatin1Char('/') + capability;
        const QFileInfo fi(entry);

        QString inspect;
        if (fi.isDir() && !fi.isSymLink())
            inspect = fi.absoluteFilePath();
        else if (fi.isSymLink())
        {
            QString target = fi.symLinkTarget();
            if (!QDir::isAbsolutePath(target))
                target = fi.absoluteDir().absoluteFilePath(target);
            inspect = remapNemoHome(target);
        }
        else
            return QString();

        return existingCapabilityDir(inspect);
    }
}

QString IconPaths::muotoShare()
{
    return QString::fromLatin1(kMuotoShare);
}

QString IconPaths::backupIconsRoot()
{
    return muotoShare() + QStringLiteral("/backup/icons");
}

QString IconPaths::packDir(const QString &packName)
{
    return QString::fromLatin1(kPackPrefix) + packShortName(packName);
}

QString IconPaths::resolvePackCapabilityDir(const QString &packRoot, const QString &capability)
{
    QString shareRoot = packRoot;
    if (!shareRoot.startsWith(QString::fromLatin1(kPackPrefix)))
        shareRoot = packDir(packRoot);

    const QString homeRoot = packHomeRoot(shareRoot);
    if (!homeRoot.isEmpty())
    {
        const QString homeCap = resolveCapabilityDir(homeRoot, capability);
        if (!homeCap.isEmpty())
            return homeCap;
    }

    return resolveCapabilityDir(shareRoot, capability);
}

bool IconPaths::packCapabilityUsable(const QString &packRoot, const QString &capability)
{
    const QString resolved = resolvePackCapabilityDir(packRoot, capability);
    if (resolved.isEmpty())
        return false;

    const QDir dir(resolved);
    return !dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot).isEmpty();
}

const QStringList &IconPaths::jollaSizes()
{
    return kJolla;
}

QString IconPaths::stockJollaIconsSourceDir(const QString &zSize)
{
    return QString::fromLatin1(kJollaRoot) + QLatin1Char('/') + zSize + QStringLiteral("/icons/");
}

QString IconPaths::liveJollaIconsDir(const QString &zSize)
{
    return stockJollaIconsSourceDir(zSize);
}
