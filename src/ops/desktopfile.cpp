#include "desktopfile.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QSaveFile>
#include <QDir>

static const QString kSection = QStringLiteral("[Desktop Entry]");

DesktopFile::DesktopFile(const QString& path)
    : _path(path), _loaded(false)
{
}

bool DesktopFile::exists() const
{
    return QFileInfo::exists(_path);
}

bool DesktopFile::load()
{
    _lines.clear();
    _loaded = false;

    QFile f(_path);
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&f);
    in.setCodec("UTF-8");

    while(!in.atEnd())
        _lines << in.readLine();

    f.close();
    _loaded = true;
    return true;
}

static int findSection(const QStringList& lines, const QString& section)
{
    for(int i = 0; i < lines.size(); ++i)
    {
        const QString s = lines[i].trimmed();
        if(s == section)
            return i;
    }
    return -1;
}

static int findKeyInSection(const QStringList& lines, int sectionIdx, const QString& key)
{
    if(sectionIdx < 0)
        return -1;

    const QString prefix = key + QStringLiteral("=");

    for(int i = sectionIdx + 1; i < lines.size(); ++i)
    {
        const QString trimmed = lines[i].trimmed();

        if(trimmed.startsWith(QLatin1Char('[')) && trimmed.endsWith(QLatin1Char(']')))
            return -1; // entered a different section

        if(trimmed.startsWith(prefix))
            return i;
    }

    return -1;
}

QString DesktopFile::value(const QString& key) const
{
    const int sec = findSection(_lines, kSection);
    if(sec < 0)
        return QString();

    const int idx = findKeyInSection(_lines, sec, key);
    if(idx < 0)
        return QString();

    const QString line = _lines[idx];
    const int eq = line.indexOf(QLatin1Char('='));
    if(eq < 0)
        return QString();

    return line.mid(eq + 1);
}

void DesktopFile::setValue(const QString& key, const QString& value)
{
    int sec = findSection(_lines, kSection);
    if(sec < 0)
    {
        if(!_lines.isEmpty() && !_lines.last().isEmpty())
            _lines << QString();
        _lines << kSection;
        sec = _lines.size() - 1;
    }

    const int idx = findKeyInSection(_lines, sec, key);
    const QString newLine = key + QStringLiteral("=") + value;

    if(idx >= 0)
    {
        _lines[idx] = newLine;
        return;
    }

    // Insert right after the section header (or its last contiguous line).
    int insertAt = sec + 1;
    while(insertAt < _lines.size())
    {
        const QString t = _lines[insertAt].trimmed();
        if(t.startsWith(QLatin1Char('[')) && t.endsWith(QLatin1Char(']')))
            break;
        insertAt++;
    }
    _lines.insert(insertAt, newLine);
}

bool DesktopFile::save()
{
    if(!_loaded)
        return false;

    // Make sure the parent directory exists.
    QFileInfo fi(_path);
    QDir().mkpath(fi.absolutePath());

    QSaveFile sf(_path);
    if(!sf.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QTextStream out(&sf);
    out.setCodec("UTF-8");

    for(int i = 0; i < _lines.size(); ++i)
    {
        out << _lines[i];
        if(i != _lines.size() - 1 || !_lines[i].isEmpty())
            out << '\n';
    }
    out.flush();

    return sf.commit();
}
