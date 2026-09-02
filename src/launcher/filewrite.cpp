#include "filewrite.h"

#include <QDebug>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace FileWrite {

bool inPlace(const QString& path, const QByteArray& content)
{
    if(path.isEmpty() || content.isEmpty())
        return false;

    const int fd = ::open(path.toLocal8Bit().constData(), O_WRONLY | O_TRUNC | O_CLOEXEC);
    if(fd < 0)
    {
        qWarning() << "muoto-launcher: could not open for write" << path << "errno=" << errno;
        return false;
    }

    qint64 written = 0;
    while(written < content.size())
    {
        const ssize_t n = ::write(fd, content.constData() + written,
                                  static_cast<size_t>(content.size() - written));
        if(n <= 0)
        {
            qWarning() << "muoto-launcher: short write on" << path << "errno=" << errno;
            ::close(fd);
            return false;
        }
        written += n;
    }

    if(::fsync(fd) != 0)
        qWarning() << "muoto-launcher: fsync failed for" << path;
    ::close(fd);
    return true;
}

} // namespace FileWrite
