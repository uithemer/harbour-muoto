#ifndef FILEWRITE_H
#define FILEWRITE_H

#include <QByteArray>
#include <QString>

namespace FileWrite {

// Replaces the contents of an existing file without replacing the inode, so
// uid, gid and mode survive. launcher-icond runs as defaultuser with
// CAP_DAC_OVERRIDE but no CAP_CHOWN: anything that creates a new inode under
// /usr/share leaves a defaultuser-owned file that rpm -V flags and that the
// daemon cannot chown back to root.
//
// Never creates the file. A vanished path is a failure, not a new file, which
// is what stops a write racing an uninstall or a re-arm from resurrecting an
// entry as a stub.
bool inPlace(const QString& path, const QByteArray& content);

} // namespace FileWrite

#endif // FILEWRITE_H
