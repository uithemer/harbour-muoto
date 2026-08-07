#include "launcherimageprovider.h"
#include "harbourthemepack.h"
#include "svgiconrender.h"

#include <QDate>
#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QTime>

namespace {

const QString kDynAssetsPath =
    QStringLiteral("/usr/share/harbour-muoto/dynamic-icons/default-dynamic-icons");

QString stripQuery(QString id)
{
    const int q = id.indexOf(QLatin1Char('?'));
    if(q >= 0)
        id = id.left(q);
    return id;
}

QString normalizePack(QString pack)
{
    if(pack.isEmpty() || pack == QLatin1String("default"))
        return QString();
    return pack;
}

int hoursAngle(const QTime& time)
{
    const int minutes = (time.hour() % 12) * 60 + time.minute();
    return 360 * minutes / (12 * 60);
}

int minutesAngle(const QTime& time)
{
    return 360 * time.minute() / 60;
}

QImage defaultClock(const QSize& requestedSize)
{
    QFile file(kDynAssetsPath + QStringLiteral("/icon-launcher-clock.svg"));
    if(!file.open(QIODevice::ReadOnly))
        return {};

    const QTime time = QTime::currentTime();
    QByteArray asset = file.readAll();
    asset.replace("rotate(125", ("rotate(" + QString::number(hoursAngle(time))).toLatin1());
    asset.replace("rotate(0", ("rotate(" + QString::number(minutesAngle(time))).toLatin1());
    return renderSvgIcon(asset, requestedSize);
}

QImage packClock(IconPack* pack, const QSize& requestedSize)
{
    if(!pack)
        return {};

    const QImage dial = pack->requestClockDialIcon(requestedSize);
    if(dial.isNull())
        return {};

    const QImage hoursHand = pack->requestHoursHandIcon(requestedSize);
    const QImage minutesHand = pack->requestMinutesHandIcon(requestedSize);
    if(hoursHand.isNull() && minutesHand.isNull())
        return dial;

    QImage image(dial);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QPoint center(image.width() / 2, image.height() / 2);
    painter.translate(center);

    const QTime time = QTime::currentTime();
    if(!hoursHand.isNull())
    {
        painter.save();
        painter.rotate(hoursAngle(time));
        painter.drawImage(QPoint(-hoursHand.width() / 2, -hoursHand.height() / 2), hoursHand);
        painter.restore();
    }

    if(!minutesHand.isNull())
    {
        painter.rotate(minutesAngle(time));
        painter.drawImage(QPoint(-minutesHand.width() / 2, -minutesHand.height() / 2), minutesHand);
    }

    return image;
}

QImage defaultCalendar(const QSize& requestedSize)
{
    QFile file(kDynAssetsPath + QStringLiteral("/icon-launcher-calendar.svg"));
    if(!file.open(QIODevice::ReadOnly))
        return {};

    QByteArray asset = file.readAll();
    const QString day = QStringLiteral(">") + QString::number(QDate::currentDate().day())
                        + QLatin1Char('<');
    asset.replace(">31<", day.toLatin1());
    return renderSvgIcon(asset, requestedSize);
}

} // namespace

LauncherImageProvider::LauncherImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage LauncherImageProvider::requestImage(const QString& id,
                                           QSize* size,
                                           const QSize& requestedSize)
{
    QImage result;
    const QString path = stripQuery(id);
    const QString type = path.section(QLatin1Char('/'), 0, 0);
    const QString rest = path.section(QLatin1Char('/'), 1);

    if(type == QLatin1String("icon-pack"))
        result = iconFromPack(rest, requestedSize);
    else if(type == QLatin1String("dyn-clock"))
        result = dynClock(rest, requestedSize);
    else if(type == QLatin1String("dyn-calendar"))
        result = dynCalendar(rest, requestedSize);
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

QImage LauncherImageProvider::dynClock(const QString& packName, const QSize& requestedSize)
{
    const QString pack = normalizePack(packName.section(QLatin1Char('/'), 0, 0));
    if(pack.isEmpty())
        return defaultClock(requestedSize);

    IconPack* iconPack = packByName(pack);
    if(!iconPack)
        return defaultClock(requestedSize);

    const QImage image = packClock(iconPack, requestedSize);
    return image.isNull() ? defaultClock(requestedSize) : image;
}

QImage LauncherImageProvider::dynCalendar(const QString& packName, const QSize& requestedSize)
{
    const QString pack = normalizePack(packName.section(QLatin1Char('/'), 0, 0));
    if(pack.isEmpty())
        return defaultCalendar(requestedSize);

    IconPack* iconPack = packByName(pack);
    if(!iconPack)
        return defaultCalendar(requestedSize);

    const QImage image = iconPack->requestCalendarIcon(QDate::currentDate(), requestedSize);
    return image.isNull() ? defaultCalendar(requestedSize) : image;
}
