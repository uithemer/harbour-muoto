#include "fontapplier.h"
#include "filelock.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QRawFont>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <QtConcurrent>

// Long enough to sit out a launcher-icond drain, short enough that a genuinely
// stuck holder still surfaces as an error.
static const int kLockWaitMs = 60 * 1000;

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
    return confDir() + QStringLiteral("/99-muoto.conf");
}

QString FontApplier::packDir(const QString& packName) const
{
    static const QString kBarePrefix = QStringLiteral("harbour-themepack-");
    QString name = packName;
    if(name.startsWith(kBarePrefix))
        name = name.mid(kBarePrefix.size());
    return QString::fromLatin1(kPackPrefix) + name;
}

QString FontApplier::stageRoot() const
{
    // ~/.local/share/fonts/muoto — Sailjail Base can read this; pack dirs
    // under harbour-themepack / ~/.themepack cannot.
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
         + QStringLiteral("/fonts/muoto");
}

QString FontApplier::stageFontDir() const
{
    return stageRoot() + QStringLiteral("/font");
}

QString FontApplier::stageNonlatinDir() const
{
    return stageRoot() + QStringLiteral("/font-nonlatin");
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

bool FontApplier::clearStage() const
{
    const QString root = stageRoot();
    if(!QFileInfo::exists(root))
        return true;
    QDir dir(root);
    if(!dir.removeRecursively())
    {
        qWarning() << "FontApplier: cannot remove staging" << root;
        return false;
    }
    return true;
}

bool FontApplier::copyDirFiles(const QString& srcDir, const QString& dstDir) const
{
    if(!QFileInfo(srcDir).isDir())
        return true;

    if(!QDir().mkpath(dstDir))
    {
        qWarning() << "FontApplier: cannot create" << dstDir;
        return false;
    }

    QDirIterator it(srcDir, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    while(it.hasNext())
    {
        it.next();
        const QFileInfo info = it.fileInfo();
        // Resolve symlinks (pack trees often link into ~/.themepack) so the
        // staged file is a real copy jailed apps can open.
        const QString src = info.canonicalFilePath();
        if(src.isEmpty() || !QFileInfo(src).isFile())
        {
            qWarning() << "FontApplier: skip unreadable" << info.absoluteFilePath();
            continue;
        }

        const QString dst = dstDir + QLatin1Char('/') + info.fileName();
        if(QFile::exists(dst) && !QFile::remove(dst))
        {
            qWarning() << "FontApplier: cannot replace" << dst;
            return false;
        }
        if(!QFile::copy(src, dst))
        {
            qWarning() << "FontApplier: copy failed" << src << "->" << dst;
            return false;
        }
    }
    return true;
}

bool FontApplier::stagePackFonts(const QString& packName) const
{
    if(!clearStage())
        return false;

    const QString root = packDir(packName);
    const QString packFont = root + QStringLiteral("/font");
    const QString packNonlatin = root + QStringLiteral("/font-nonlatin");

    if(!copyDirFiles(packFont, stageFontDir()))
        return false;
    if(!copyDirFiles(packNonlatin, stageNonlatinDir()))
        return false;

    // At least one latin weight must have been staged.
    if(!QFileInfo(stageFontDir()).isDir()
       || QDir(stageFontDir()).entryList(QDir::Files).isEmpty())
    {
        qWarning() << "FontApplier: no fonts staged from" << packFont;
        return false;
    }
    return true;
}

QString FontApplier::buildConfXml(const QString& packName,
                                  const QString& weightBasename,
                                  const QString& latinFamily) const
{
    const QString fontDir = stageFontDir();
    const QString nonlatinDir = stageNonlatinDir();

    QString xml;
    xml.reserve(2048);
    xml += QStringLiteral("<?xml version=\"1.0\"?>\n");
    xml += QStringLiteral("<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n");
    xml += QStringLiteral("<fontconfig>\n");
    xml += QStringLiteral("    <description>Muoto alias for theme '")
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
    // Called from a worker thread — blocking wait is fine here.
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(QStringLiteral("fc-cache"), QStringList() << QStringLiteral("-f"));
    p.waitForStarted();
    p.waitForFinished(15000);
}

void FontApplier::applyFromPack(const QString& packName, const QString& weightBasename)
{
    // Staging + fc-cache are slow; keep the Silica event loop responsive.
    QtConcurrent::run(this, &FontApplier::applyFromPackWorker, packName, weightBasename);
}

void FontApplier::restoreFonts()
{
    QtConcurrent::run(this, &FontApplier::restoreFontsWorker);
}

void FontApplier::applyFromPackWorker(const QString& packName, const QString& weightBasename)
{
    // Always emit applied(packName) at the end (success or logged failure)
    // so callers' one-shot connections drain and the busy spinner clears.
    // Mirrors IconApplier's "log-and-continue" error policy.
    // Release FileLock before emit so a follow-up ApplyIcons (same lock)
    // is not rejected as busy when QML chains font→icon.
    QString err;
    bool ok = false;
    {
        // Waits rather than failing: this runs on a worker thread, and
        // launcher-icond now holds the shared sentinel for a whole queue drain,
        // so a user's font apply should sit out someone else's work instead of
        // returning "busy".
        FileLock lk;
        if(!FileLock::waitFor(&lk, kLockWaitMs))
        {
            err = QStringLiteral("busy");
        }
        else if(packName.isEmpty() || weightBasename.isEmpty())
        {
            err = QStringLiteral("empty pack or weight");
        }
        else
        {
            const QString ttfPath = packDir(packName) + QStringLiteral("/font/")
                                  + weightBasename + QStringLiteral(".ttf");
            const QString family = familyFromTtf(ttfPath);
            if(family.isEmpty())
            {
                err = QStringLiteral("could not read font family from %1").arg(ttfPath);
            }
            else if(!stagePackFonts(packName))
            {
                clearStage();
                err = QStringLiteral("failed to stage fonts for Sailjail");
            }
            else
            {
                const QString xml = buildConfXml(packName, weightBasename, family);
                if(!writeConf(xml))
                {
                    clearStage();
                    err = QStringLiteral("failed to write fontconfig conf");
                }
                else
                {
                    runFcCache();
                    ok = true;
                }
            }
        }
    }
    if(!ok)
        emit error(err);
    emit applied(packName);
}

void FontApplier::restoreFontsWorker()
{
    QString err;
    bool ok = false;
    {
        FileLock lk;
        if(!FileLock::waitFor(&lk, kLockWaitMs))
        {
            err = QStringLiteral("busy");
        }
        else if(!removeConf())
        {
            err = QStringLiteral("failed to remove fontconfig conf");
        }
        else if(!clearStage())
        {
            err = QStringLiteral("failed to remove staged fonts");
        }
        else
        {
            runFcCache();
            ok = true;
        }
    }
    if(!ok)
        emit error(err);
    emit restored();
}
