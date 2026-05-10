#include "filelock.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>

#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

QString FileLock::defaultLockPath()
{
    return QStringLiteral("/usr/share/sailfishos-uithemer/icon-backup.lock");
}

FileLock::FileLock(const QString& path, bool blocking)
    : _fd(-1), _path(path)
{
    QFileInfo fi(_path);
    QDir().mkpath(fi.absolutePath());

    // O_CREAT lets us seed the sentinel on first run; the file content is
    // irrelevant. Mode 0644 so the helper (root) and the GUI (root) can both
    // open it; users never touch it.
    _fd = ::open(_path.toLocal8Bit().constData(),
                 O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if(_fd < 0)
    {
        qWarning() << "FileLock: open failed" << _path << "errno=" << errno;
        return;
    }

    int op = LOCK_EX;
    if(!blocking)
        op |= LOCK_NB;

    if(::flock(_fd, op) != 0)
    {
        qWarning() << "FileLock: flock failed" << _path << "errno=" << errno;
        ::close(_fd);
        _fd = -1;
        return;
    }
}

FileLock::~FileLock()
{
    if(_fd >= 0)
    {
        ::flock(_fd, LOCK_UN);
        ::close(_fd);
    }
}
