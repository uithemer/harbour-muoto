#include "devicelockstatus.h"
#include "dynamicicon.h"
#include "harbourthemepack.h"
#include "iconprovider.h"
#include "svgiconrender.h"

#include <MGConfItem>
#include <QDate>
#include <QFile>
#include <QScopedPointer>
#include <QTimer>

namespace {

class CalendarIconRenderer
{
public:
    virtual ~CalendarIconRenderer() = default;
    virtual QImage requestImage(const QDate& date, const QSize& requestedSize) = 0;
};

class DefaultCalendarIconRenderer : public CalendarIconRenderer
{
public:
    DefaultCalendarIconRenderer()
    {
        QFile file(QStringLiteral(ASSETS_PATH "/icon-launcher-calendar.svg"));
        if(file.open(QIODevice::ReadOnly))
            m_asset = file.readAll();
    }

    QImage requestImage(const QDate& date, const QSize& requestedSize) override
    {
        return renderSvgIcon(getSvgData(date), requestedSize);
    }

private:
    QByteArray getSvgData(const QDate& date)
    {
        QByteArray asset(m_asset);
        const QString day = QStringLiteral(">") + QString::number(date.day()) + QLatin1Char('<');
        asset.replace(">31<", day.toLatin1());
        return asset;
    }

    QByteArray m_asset;
};

class IconpackCalendarIconRenderer : public CalendarIconRenderer
{
public:
    explicit IconpackCalendarIconRenderer(const QString& name)
    {
        m_iconPack = HarbourThemePack::byShortName(name);
    }

    QImage requestImage(const QDate& date, const QSize& requestedSize) override
    {
        if(!m_iconPack)
            return {};
        return m_iconPack->requestCalendarIcon(date, requestedSize);
    }

private:
    IconPack* m_iconPack = nullptr;
};

class CalendarIconProvider : public IconProvider
{
    Q_OBJECT

public:
    explicit CalendarIconProvider(QObject* parent)
        : IconProvider(parent)
        , m_activePackConf(QStringLiteral("/apps/harbour-muoto/activeIconPack"))
    {
        updateRenderer();
        connect(&m_activePackConf, &MGConfItem::valueChanged, this, &CalendarIconProvider::updateRenderer);
        connect(&m_activePackConf, &MGConfItem::valueChanged, this, &IconProvider::imageUpdated);
        connect(&m_timer, &QTimer::timeout, this, &IconProvider::imageUpdated);
        connect(&m_devicelockStatus, &DevicelockStatus::locked, &m_timer, &QTimer::stop);
        connect(&m_devicelockStatus, &DevicelockStatus::unlocked, this, [this]() {
            if(!m_timer.isActive())
                m_timer.start();
            emit imageUpdated();
        });
        m_timer.start(60 * 60 * 1000);
    }

    QImage requestImage(const QSize& requestedSize) override
    {
        return m_renderer->requestImage(QDate::currentDate(), requestedSize);
    }

private slots:
    void updateRenderer()
    {
        QString iconPackName = m_activePackConf.value(QStringLiteral("default")).toString();
        if(iconPackName == QLatin1String("default"))
            iconPackName.clear();

        if(iconPackName.isEmpty())
            m_renderer.reset(new DefaultCalendarIconRenderer);
        else
            m_renderer.reset(new IconpackCalendarIconRenderer(iconPackName));
    }

private:
    QTimer m_timer;
    DevicelockStatus m_devicelockStatus;
    MGConfItem m_activePackConf;
    QScopedPointer<CalendarIconRenderer> m_renderer;
};

} // namespace

class CalendarDynamicIcon : public DynamicIcon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit CalendarDynamicIcon(QObject* parent = nullptr)
        : DynamicIcon(QStringLiteral("jolla-calendar"), QStringLiteral("muoto-jolla-calendar"), parent)
    {
    }

protected:
    IconProvider* createIconProvider(QObject* parent) override
    {
        return new CalendarIconProvider(parent);
    }
};

REGISTER_DYNAMIC_ICON(CalendarDynamicIcon)

#include "calendar.moc"
