#ifndef DESKTOPFILE_H
#define DESKTOPFILE_H

#include <QString>
#include <QStringList>

// Minimal XDG .desktop reader/writer.
// - Preserves line order, blank lines and comments.
// - Operates only on the [Desktop Entry] section by default (Section "Desktop Entry").
// - Writes use a temporary file + rename(2) for atomicity.
class DesktopFile
{
public:
    explicit DesktopFile(const QString& path);

    bool exists() const;
    bool load();

    // Returns the value of the given key in [Desktop Entry], or QString() if absent.
    QString value(const QString& key) const;

    // Replaces the value of the given key in [Desktop Entry].
    // If the key is missing, appends it under that section.
    void setValue(const QString& key, const QString& value);

    // Writes the file atomically. Returns true on success.
    bool save();

    QString path() const { return _path; }

private:
    QString _path;
    QStringList _lines; // raw lines (without trailing newline)
    bool _loaded;
};

#endif // DESKTOPFILE_H
