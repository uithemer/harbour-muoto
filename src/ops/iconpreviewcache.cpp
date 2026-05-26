#include "iconpreviewcache.h"

#include <QMutexLocker>

IconPreviewCache& IconPreviewCache::instance()
{
    static IconPreviewCache s;
    return s;
}

void IconPreviewCache::put(const QString& packName, const QImage& img)
{
    QMutexLocker lock(&this->_m);
    if(img.isNull())
        this->_cache.remove(packName);
    else
        this->_cache.insert(packName, img);
}

QImage IconPreviewCache::get(const QString& packName) const
{
    QMutexLocker lock(&this->_m);
    return this->_cache.value(packName);
}

void IconPreviewCache::clear()
{
    QMutexLocker lock(&this->_m);
    this->_cache.clear();
}
