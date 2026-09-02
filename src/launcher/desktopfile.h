#ifndef DESKTOPFILE_H
#define DESKTOPFILE_H

#include <glib.h>
#include <QByteArray>
#include <QString>

// Replaces DesktopEntry, which failed open: an unreadable or missing file left
// an empty GKeyFile behind with no error flag, so setIcon()+save() replaced the
// entry with a two-line stub and Lipstick lost the launcher item.
//
// Two rules make that impossible:
//
//   1. Fail closed. loaded() reflects the read; save() refuses unless the file
//      was read, still exists, and the serialised result still carries Type and
//      Name in [Desktop Entry].
//   2. Write in place. launcher-icond runs as defaultuser with CAP_DAC_OVERRIDE
//      but no CAP_CHOWN, so a temp+rename would create a defaultuser-owned inode
//      it cannot chown back to root. open(O_WRONLY|O_TRUNC) on the existing inode
//      keeps uid, gid and mode; O_CREAT is never used, so a vanished path is a
//      refusal rather than a new stub.
class DesktopFile
{
public:
    explicit DesktopFile(const QString& path);
    ~DesktopFile();

    bool loaded() const { return m_loaded; }

    QString icon() const;
    void setIcon(const QString& icon);
    bool save();

private:
    DesktopFile(const DesktopFile&) = delete;
    DesktopFile& operator=(const DesktopFile&) = delete;

    void captureBackupOnce() const;

    QString m_path;
    QByteArray m_original;
    bool m_loaded = false;
    bool m_hasChanges = false;
    GKeyFile* m_keyFile;
};

#endif // DESKTOPFILE_H
