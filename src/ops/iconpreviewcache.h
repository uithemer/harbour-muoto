#ifndef ICONPREVIEWCACHE_H
#define ICONPREVIEWCACHE_H

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QString>

// Process-wide, thread-safe in-memory cache for the ConfirmPage / Cover icon
// preview. Built by IconApplier::buildPreview and read by IconPreviewProvider
// when QML resolves an image://muoto/preview/<packName> URL.
//
// The cache is intentionally a plain C++ singleton (no QObject parent) so it
// outlives any Qt object hierarchy and is leak-free at process exit.
class IconPreviewCache
{
public:
    static IconPreviewCache& instance();

    void put(const QString& packName, const QImage& img);
    QImage get(const QString& packName) const;
    void clear();

private:
    IconPreviewCache() {}
    IconPreviewCache(const IconPreviewCache&);
    IconPreviewCache& operator=(const IconPreviewCache&);

    mutable QMutex _m;
    QHash<QString, QImage> _cache;
};

#endif // ICONPREVIEWCACHE_H
