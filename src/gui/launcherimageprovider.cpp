#include "launcherimageprovider.h"
#include "harbourthemepack.h"

#include <QDebug>

LauncherImageProvider::LauncherImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage LauncherImageProvider::requestImage(const QString& id,
                                           QSize* size,
                                           const QSize& requestedSize)
{
    QImage result;
    const QString type = id.section(QLatin1Char('/'), 0, 0);
    const QString path = id.section(QLatin1Char('/'), 1);

    if(type == QLatin1String("icon-pack"))
        result = iconFromPack(path, requestedSize);
    else
        qWarning() << "muoto: invalid launcher image type" << type;

    if(size)
        *size = result.size();

    return result;
}

IconPack* LauncherImageProvider::packByName(const QString& name)
{
    if(m_iconPacks.contains(name))
        return m_iconPacks.value(name);

    IconPack* pack = HarbourThemePack::byShortName(name);
    if(pack)
        m_iconPacks.insert(name, pack);
    return pack;
}

QImage LauncherImageProvider::iconFromPack(const QString& path, const QSize& requestedSize)
{
    const QString iconPackName = path.section(QLatin1Char('/'), 0, 0);
    QString iconId = path.section(QLatin1Char('/'), 1);
    if(iconId.isEmpty())
        return {};

    IconPack* pack = packByName(iconPackName);
    if(!pack)
        return {};

    return pack->requestIcon(iconId, requestedSize);
}
