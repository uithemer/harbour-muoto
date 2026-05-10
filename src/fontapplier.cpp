#include "fontapplier.h"
#include "filelock.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QRawFont>
#include <QProcess>
#include <QDebug>

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

const char* FontApplier::kConfDir =
    "/home/defaultuser/.config/fontconfig/conf.d";
const char* FontApplier::kConfPath =
    "/home/defaultuser/.config/fontconfig/conf.d/99-uithemer.conf";

static const char* kPackPrefix = "/usr/share/harbour-themepack-";
static const char* kSailFamily   = "Sail Sans Pro Light";
static const char* kSailHeading  = "Sail Sans Pro";

namespace
{
    // Map well-known non-latin filenames in <pack>/font-nonlatin/ to the
    // fontconfig <lang> tag used by Qt / Lipstick.
    struct LangEntry { const char* file; const char* lang; };
    static const LangEntry kLangMap[] = {
        { "Arabic.ttf",      "ar" },
        { "Devanagari.ttf",  "hi" },
        { "Chinese.ttc",     "zh" },
        { "Japanese.ttf",    "ja" },
        { "Hebrew.ttf",      "he" },
        { "Thai.ttf",        "th" },
    };
    static const int kLangMapSize = sizeof(kLangMap) / sizeof(kLangMap[0]);

    // Minimal XML escape for values we splice into <family>...</family>
    // and <description>...</description>. Pack and weight strings come from
    // filesystem layout so they are normally safe, but be defensive.
    QString xmlEscape(const QString& s)
    {
        QString r = s;
        r.replace(QLatin1Char('&'),  QStringLiteral("&amp;"));
        r.replace(QLatin1Char('<'),  QStringLiteral("&lt;"));
        r.replace(QLatin1Char('>'),  QStringLiteral("&gt;"));
        r.replace(QLatin1Char('"'),  QStringLiteral("&quot;"));
        r.replace(QLatin1Char('\''), QStringLiteral("&apos;"));
        return r;
    }
}

FontApplier::FontApplier(QObject* parent) : QObject(parent)
{
}

QString FontApplier::packDir(const QString& packName) const
{
    static const QString kBarePrefix = QStringLiteral("harbour-themepack-");
    QString name = packName;
    if(name.startsWith(kBarePrefix))
        name = name.mid(kBarePrefix.size());
    return QString::fromLatin1(kPackPrefix) + name;
}

QString FontApplier::familyFromTtf(const QString& path) const
{
    if(!QFileInfo::exists(path))
        return QString();
    // Pixel size is required by QRawFont but irrelevant for family lookup.
    QRawFont rf(path, 10);
    if(!rf.isValid())
        return QString();
    return rf.familyName();
}

void FontApplier::chownToDefaultUser(const QString& path) const
{
    struct passwd* pw = getpwnam("defaultuser");
    if(!pw)
        return;
    if(chown(path.toLocal8Bit().constData(), pw->pw_uid, pw->pw_gid) != 0)
        qDebug() << "FontApplier: chown failed" << path;
}

QString FontApplier::buildConfXml(const QString& packName,
                                  const QString& weightBasename,
                                  const QString& latinFamily) const
{
    const QString root = packDir(packName);
    const QString fontDir     = root + QStringLiteral("/font");
    const QString nonlatinDir = root + QStringLiteral("/font-nonlatin");

    QString xml;
    xml.reserve(2048);
    xml += QStringLiteral("<?xml version=\"1.0\"?>\n");
    xml += QStringLiteral("<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n");
    xml += QStringLiteral("<fontconfig>\n");
    xml += QStringLiteral("    <description>UI Themer alias for theme '")
         + xmlEscape(packName) + QStringLiteral("' weight '")
         + xmlEscape(weightBasename) + QStringLiteral("'</description>\n");

    if(QFileInfo(fontDir).isDir())
        xml += QStringLiteral("    <dir>") + xmlEscape(fontDir)
             + QStringLiteral("</dir>\n");
    if(QFileInfo(nonlatinDir).isDir())
        xml += QStringLiteral("    <dir>") + xmlEscape(nonlatinDir)
             + QStringLiteral("</dir>\n");

    xml += QStringLiteral("\n");
    xml += QStringLiteral("    <alias binding=\"strong\">\n");
    xml += QStringLiteral("        <family>") + xmlEscape(QString::fromLatin1(kSailFamily))
         + QStringLiteral("</family>\n");
    xml += QStringLiteral("        <prefer><family>") + xmlEscape(latinFamily)
         + QStringLiteral("</family></prefer>\n");
    xml += QStringLiteral("    </alias>\n");
    xml += QStringLiteral("    <alias binding=\"strong\">\n");
    xml += QStringLiteral("        <family>") + xmlEscape(QString::fromLatin1(kSailHeading))
         + QStringLiteral("</family>\n");
    xml += QStringLiteral("        <prefer><family>") + xmlEscape(latinFamily)
         + QStringLiteral("</family></prefer>\n");
    xml += QStringLiteral("    </alias>\n");

    xml += QStringLiteral("\n");
    xml += QStringLiteral("    <selectfont>\n");
    xml += QStringLiteral("        <rejectfont><glob>/usr/share/fonts/sail-sans-pro</glob></rejectfont>\n");
    xml += QStringLiteral("    </selectfont>\n");

    if(QFileInfo(nonlatinDir).isDir())
    {
        for(int i = 0; i < kLangMapSize; ++i)
        {
            const QString filePath = nonlatinDir + QLatin1Char('/')
                                   + QString::fromLatin1(kLangMap[i].file);
            const QString fam = familyFromTtf(filePath);
            if(fam.isEmpty())
                continue;
            xml += QStringLiteral("\n");
            xml += QStringLiteral("    <match target=\"pattern\">\n");
            xml += QStringLiteral("        <test name=\"lang\"><string>")
                 + QString::fromLatin1(kLangMap[i].lang)
                 + QStringLiteral("</string></test>\n");
            xml += QStringLiteral("        <edit name=\"family\" mode=\"prepend\" binding=\"strong\"><string>")
                 + xmlEscape(fam)
                 + QStringLiteral("</string></edit>\n");
            xml += QStringLiteral("    </match>\n");
        }
    }

    xml += QStringLiteral("</fontconfig>\n");
    return xml;
}

bool FontApplier::writeConf(const QString& xml)
{
    const QString dir = QString::fromLatin1(kConfDir);
    QDir().mkpath(dir);
    chownToDefaultUser(dir);
    // Also chown the parent .config and fontconfig dirs if we just
    // created them, so the per-user fontconfig cache (~/.cache/fontconfig)
    // can be written by defaultuser later.
    chownToDefaultUser(QStringLiteral("/home/defaultuser/.config"));
    chownToDefaultUser(QStringLiteral("/home/defaultuser/.config/fontconfig"));

    const QString path = QString::fromLatin1(kConfPath);
    QSaveFile f(path);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "FontApplier: cannot open" << path << f.errorString();
        return false;
    }
    const QByteArray bytes = xml.toUtf8();
    if(f.write(bytes) != bytes.size())
    {
        qWarning() << "FontApplier: short write" << path;
        return false;
    }
    if(!f.commit())
    {
        qWarning() << "FontApplier: commit failed" << path << f.errorString();
        return false;
    }
    chmod(path.toLocal8Bit().constData(), 0644);
    chownToDefaultUser(path);
    return true;
}

bool FontApplier::removeConf()
{
    const QString path = QString::fromLatin1(kConfPath);
    if(!QFileInfo::exists(path))
        return true;
    if(!QFile::remove(path))
    {
        qWarning() << "FontApplier: cannot remove" << path;
        return false;
    }
    return true;
}

void FontApplier::runFcCache()
{
    // Run as defaultuser so the per-user cache (~/.cache/fontconfig) is
    // owned by defaultuser, not root.
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(QStringLiteral("su"),
            QStringList()
                << QStringLiteral("-")
                << QStringLiteral("defaultuser")
                << QStringLiteral("-c")
                << QStringLiteral("fc-cache -f"));
    p.waitForStarted();
    p.waitForFinished(15000);
}

void FontApplier::applyFromPack(const QString& packName, const QString& weightBasename)
{
    // Always emit applied(packName) at the end (success or logged failure)
    // so callers' one-shot connections drain and the busy spinner clears.
    // Mirrors IconApplier's "log-and-continue" error policy.
    FileLock lk;
    if(!lk.isHeld())
    {
        emit error(QStringLiteral("could not acquire lock"));
        emit applied(packName);
        return;
    }

    if(packName.isEmpty() || weightBasename.isEmpty())
    {
        emit error(QStringLiteral("empty pack or weight"));
        emit applied(packName);
        return;
    }

    const QString ttfPath = packDir(packName) + QStringLiteral("/font/")
                          + weightBasename + QStringLiteral(".ttf");
    const QString family = familyFromTtf(ttfPath);
    if(family.isEmpty())
    {
        emit error(QStringLiteral("could not read font family from %1").arg(ttfPath));
        emit applied(packName);
        return;
    }

    const QString xml = buildConfXml(packName, weightBasename, family);
    if(!writeConf(xml))
    {
        emit error(QStringLiteral("failed to write fontconfig conf"));
        emit applied(packName);
        return;
    }
    runFcCache();
    emit applied(packName);
}

void FontApplier::restoreFonts()
{
    FileLock lk;
    if(!lk.isHeld())
    {
        emit error(QStringLiteral("could not acquire lock"));
        emit restored();
        return;
    }
    if(!removeConf())
    {
        emit error(QStringLiteral("failed to remove fontconfig conf"));
        emit restored();
        return;
    }
    runFcCache();
    emit restored();
}

void FontApplier::recoveryFonts()
{
    // With the fontconfig approach there is nothing to reinstall: stock
    // fonts under /usr/share/fonts are never modified. Recovery is
    // equivalent to restore.
    restoreFonts();
}
