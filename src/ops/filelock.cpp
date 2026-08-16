#include "filelock.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QThread>

#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

QString FileLock::defaultLockPath()
{
    return QStringLiteral("/usr/share/harbour-muoto/icon-ops.lock");
}

bool FileLock::tryProbe(const QString& path)
{
    FileLock lk(path, false);
    return lk.isHeld();
}

bool FileLock::waitFor(FileLock* lock, int timeoutMs, const QString& path)
{
    if(!lock)
        return false;

    const int stepMs = 200;
    int waited = 0;
    while(true)
    {
        if(lock->isHeld())
            return true;
        if(waited >= timeoutMs)
        {
            qWarning() << "FileLock: gave up waiting for" << path << "after" << waited << "ms";
            return false;
        }
        QThread::msleep(stepMs);
        waited += stepMs;
        lock->reacquire();
    }
}

void FileLock::reacquire()
{
    if(_fd >= 0)
        return;

    _fd = ::open(_path.toLocal8Bit().constData(), O_RDWR | O_CREAT | O_CLOEXEC, 0666);
    if(_fd < 0)
        return;
    if(::flock(_fd, LOCK_EX | LOCK_NB) != 0)
    {
        ::close(_fd);
        _fd = -1;
    }
}

FileLock::FileLock(const QString& path, bool blocking)
    : _fd(-1), _path(path)
{
    QFileInfo fi(_path);
    QDir().mkpath(fi.absolutePath());

    // Mode 0666 so defaultuser (GUI probe) and root (helperd) can open O_RDWR.
    _fd = ::open(_path.toLocal8Bit().constData(),
                 O_RDWR | O_CREAT | O_CLOEXEC, 0666);
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
        if(!blocking && errno == EWOULDBLOCK)
        {
            ::close(_fd);
            _fd = -1;
            return;
        }
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
