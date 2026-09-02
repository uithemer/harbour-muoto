#include "devicelockstatus.h"
#include "dynamicicon.h"
#include "harbourthemepack.h"
#include "iconprovider.h"
#include "svgiconrender.h"

#include <MGConfItem>
#include <QPainter>
#include <QFile>
#include <QScopedPointer>
#include <QTime>
#include <QTimer>

namespace {

class ClockIconRenderer
{
public:
    virtual ~ClockIconRenderer() = default;
    virtual QImage requestImage(int hoursAngle, int minutesAngle, const QSize& requestedSize) = 0;
};

class DefaultClockIconRenderer : public ClockIconRenderer
{
public:
    DefaultClockIconRenderer()
    {
        QFile file(QStringLiteral(ASSETS_PATH "/icon-launcher-clock.svg"));
        if(file.open(QIODevice::ReadOnly))
            m_asset = file.readAll();
    }

    QImage requestImage(int hoursAngle, int minutesAngle, const QSize& requestedSize) override
    {
        return renderSvgIcon(getSvgData(hoursAngle, minutesAngle), requestedSize);
    }

private:
    QByteArray getSvgData(int hoursAngle, int minutesAngle)
    {
        QByteArray asset(m_asset);
        asset.replace("rotate(125", ("rotate(" + QString::number(hoursAngle)).toLatin1());
        asset.replace("rotate(0", ("rotate(" + QString::number(minutesAngle)).toLatin1());
        return asset;
    }

    QByteArray m_asset;
};

class IconpackClockIconRenderer : public ClockIconRenderer
{
public:
    explicit IconpackClockIconRenderer(const QString& name)
    {
        // byShortName() hands back a fresh allocation each call, so this owns it.
        // Without the delete below, every pack change leaked a whole pack index.
        m_iconPack = HarbourThemePack::byShortName(name);
    }

    ~IconpackClockIconRenderer() override
    {
        delete m_iconPack;
    }

    QImage requestImage(int hoursAngle, int minutesAngle, const QSize& requestedSize) override
    {
        if(!m_iconPack)
            return {};

        if(m_lastRequestedSize != requestedSize)
        {
            m_lastRequestedSize = requestedSize;
            m_clockDial = m_iconPack->requestClockDialIcon(requestedSize);
            m_hoursHand = m_iconPack->requestHoursHandIcon(requestedSize);
            m_minutesHand = m_iconPack->requestMinutesHandIcon(requestedSize);
        }

        QImage image(m_clockDial);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        const QPoint center(image.width() / 2, image.height() / 2);
        painter.translate(center);

        painter.save();
        painter.rotate(hoursAngle);
        painter.drawImage(QPoint(-m_hoursHand.width() / 2, -m_hoursHand.height() / 2), m_hoursHand);
        painter.restore();

        painter.rotate(minutesAngle);
        painter.drawImage(QPoint(-m_minutesHand.width() / 2, -m_minutesHand.height() / 2), m_minutesHand);

        return image;
    }

private:
    IconPack* m_iconPack = nullptr;
    QSize m_lastRequestedSize;
    QImage m_clockDial;
    QImage m_hoursHand;
    QImage m_minutesHand;
};

class ClockIconProvider : public IconProvider
{
    Q_OBJECT

public:
    explicit ClockIconProvider(QObject* parent)
        : IconProvider(parent)
        , m_activePackConf(QStringLiteral("/apps/harbour-muoto/activeIconPack"))
    {
        updateRenderer();
        connect(&m_activePackConf, &MGConfItem::valueChanged, this, &ClockIconProvider::updateRenderer);
        connect(&m_activePackConf, &MGConfItem::valueChanged, this, &IconProvider::imageUpdated);
        connect(&m_timer, &QTimer::timeout, this, &IconProvider::imageUpdated);
        connect(&m_devicelockStatus, &DevicelockStatus::locked, &m_timer, &QTimer::stop);
        connect(&m_devicelockStatus, &DevicelockStatus::unlocked, this, [this]() {
            if(!m_timer.isActive())
                m_timer.start();
            emit imageUpdated();
        });
        m_timer.start(60 * 1000);
    }

    QImage requestImage(const QSize& requestedSize) override
    {
        const QTime time = QTime::currentTime();
        return m_renderer->requestImage(hoursAngle(time), minutesAngle(time), requestedSize);
    }

private slots:
    void updateRenderer()
    {
        QString iconPackName = m_activePackConf.value(QStringLiteral("default")).toString();
        if(iconPackName == QLatin1String("default"))
            iconPackName.clear();

        if(iconPackName.isEmpty())
            m_renderer.reset(new DefaultClockIconRenderer);
        else
            m_renderer.reset(new IconpackClockIconRenderer(iconPackName));
    }

private:
    static int hoursAngle(const QTime& time)
    {
        const int minutes = (time.hour() % 12) * 60 + time.minute();
        return 360 * minutes / (12 * 60);
    }

    static int minutesAngle(const QTime& time)
    {
        return 360 * time.minute() / 60;
    }

    QTimer m_timer;
    DevicelockStatus m_devicelockStatus;
    MGConfItem m_activePackConf;
    QScopedPointer<ClockIconRenderer> m_renderer;
};

} // namespace

class ClockDynamicIcon : public DynamicIcon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit ClockDynamicIcon(QObject* parent = nullptr)
        : DynamicIcon(QStringLiteral("jolla-clock"), QStringLiteral("muoto-jolla-clock"), parent)
    {
    }

protected:
    IconProvider* createIconProvider(QObject* parent) override
    {
        return new ClockIconProvider(parent);
    }
};

REGISTER_DYNAMIC_ICON(ClockDynamicIcon)

#include "clock.moc"
