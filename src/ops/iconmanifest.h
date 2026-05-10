#ifndef ICONMANIFEST_H
#define ICONMANIFEST_H

#include <QString>
#include <QHash>

// JSON manifest of themed .desktop entries.
// Format (file at /usr/share/sailfishos-uithemer/icon-backup.json):
//   {
//     "version": 1,
//     "active_icon_pack": "mytheme" | null,
//     "entries": {
//       "/usr/share/applications/foo.desktop": {
//         "original_icon": "...",
//         "themed_icon": "...",
//         "kind": "native" | "apk"
//       },
//       ...
//     }
//   }
class IconManifest
{
public:
    struct Entry {
        QString originalIcon;
        QString themedIcon;
        QString kind; // "native" or "apk"
    };

    explicit IconManifest(const QString& path);

    bool load();
    bool save() const;

    QString activeIconPack() const { return _activeIconPack; }
    void setActiveIconPack(const QString& v) { _activeIconPack = v; }

    QHash<QString, Entry> entries() const { return _entries; }
    bool contains(const QString& path) const { return _entries.contains(path); }
    Entry entry(const QString& path) const { return _entries.value(path); }

    void setEntry(const QString& path, const Entry& e) { _entries[path] = e; }
    void removeEntry(const QString& path) { _entries.remove(path); }
    void clearEntries() { _entries.clear(); }

    QString path() const { return _path; }

private:
    QString _path;
    int _version;
    QString _activeIconPack;
    QHash<QString, Entry> _entries;
};

#endif // ICONMANIFEST_H
