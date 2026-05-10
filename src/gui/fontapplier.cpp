#include "fontapplier.h"
#include "filelock.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QRawFont>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>

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

QString FontApplier::confDir() const
{
    // Resolves to $XDG_CONFIG_HOME/fontconfig/conf.d, falling back to
    // ~/.config/fontconfig/conf.d when XDG_CONFIG_HOME is unset (which
    // is the SFOS default for defaultuser). 2.6.0 dropped the
    // hard-coded /home/defaultuser path: with the GUI now running as
    // defaultuser, QStandardPaths gives the correct directory without
    // having to assume the user name.
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
         + QStringLiteral("/fontconfig/conf.d");
}

QString FontApplier::confPath() const
{
    return confDir() + QStringLiteral("/99-uithemer.conf");
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
    QDir().mkpath(confDir());

    const QString path = confPath();
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
    return true;
}

bool FontApplier::removeConf()
{
    const QString path = confPath();
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
    // 2.6.0: with the GUI running as defaultuser, fc-cache writes into
    // the current user's ~/.cache/fontconfig directly. No `su -` shell.
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(QStringLiteral("fc-cache"), QStringList() << QStringLiteral("-f"));
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
