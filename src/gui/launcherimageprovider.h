#ifndef LAUNCHERIMAGEPROVIDER_H
#define LAUNCHERIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QHash>

class IconPack;

class LauncherImageProvider : public QQuickImageProvider
{
public:
    LauncherImageProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    QImage iconFromPack(const QString& path, const QSize& requestedSize);
    QImage dynClock(const QString& packName, const QSize& requestedSize);
    QImage dynCalendar(const QString& packName, const QSize& requestedSize);
    IconPack* packByName(const QString& name);

    QHash<QString, IconPack*> m_iconPacks;
};

#endif // LAUNCHERIMAGEPROVIDER_H
