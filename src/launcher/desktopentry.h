#ifndef DESKTOPENTRY_H
#define DESKTOPENTRY_H

#include <glib.h>
#include <QScopedPointer>
#include <QString>

class MDesktopEntry;

class DesktopEntry
{
public:
    explicit DesktopEntry(const QString& path);
    ~DesktopEntry();

    QString name();
    QString icon();
    void setIcon(const QString& icon);
    bool save();

private:
    QString m_path;
    bool m_hasChanges;
    GKeyFile* m_keyFile;
    QScopedPointer<MDesktopEntry> m_desktopEntry;
};

#endif // DESKTOPENTRY_H
