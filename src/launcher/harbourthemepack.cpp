#include "harbourthemepack.h"

#include "iconpaths.h"
#include "svgiconrender.h"

#include <silicatheme.h>
#include <QCollator>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>

namespace {

void loadIconsFromPath(const QString& path, const QString& scheme, QHash<QString, QString>& icons)
{
    QDir dir(path);
    const QFileInfoList entries = dir.entryInfoList({QStringLiteral("*.png")}, QDir::Files);
    for(const QFileInfo& entry : entries)
    {
        if(!icons.contains(entry.completeBaseName()))
            icons.insert(entry.completeBaseName(), scheme);
    }
}

void loadIcons(const QString& basePath,
               const QString& primaryScheme,
               const QString& subPath,
               QHash<QString, QString>& icons)
{
    const QString primaryPath = basePath + primaryScheme + subPath;
    if(QFile::exists(primaryPath))
        loadIconsFromPath(primaryPath, primaryScheme, icons);

    QDir dir(basePath);
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(entries.begin(), entries.end(), [&](const QString& s1, const QString& s2) {
        return collator.compare(s1, s2) > 0;
    });

    for(const QString& entry : entries)
        loadIconsFromPath(dir.absoluteFilePath(entry) + subPath, entry, icons);
}

} // namespace

HarbourThemePack::HarbourThemePack(const QString& path, QObject* parent)
    : IconPack(QFileInfo(path).fileName().remove(QStringLiteral("harbour-themepack-")), parent)
    , m_path(path)
{
    const qreal pixelRatio = Silica::Theme::instance()->pixelRatio();
    QString scale = QStringLiteral("z") + QString::number(pixelRatio, 'f', 2);
    if(scale.endsWith(QLatin1Char('0')))
        scale.chop(1);

    const qreal iconSize = Silica::Theme::instance()->iconSizeLauncher();
    const QString size = QStringLiteral("%1x%1").arg(QString::number(iconSize, 'f', 0));

    const QString jollaRoot = IconPaths::resolvePackCapabilityDir(m_path, QStringLiteral("jolla"));
    const QString nativeRoot = IconPaths::resolvePackCapabilityDir(m_path, QStringLiteral("native"));
    const QString apkRoot = IconPaths::resolvePackCapabilityDir(m_path, QStringLiteral("apk"));

    if(!jollaRoot.isEmpty())
        loadIcons(jollaRoot + QLatin1Char('/'), scale, QStringLiteral("/icons"), m_jollaIcons);
    if(!nativeRoot.isEmpty())
        loadIcons(nativeRoot + QLatin1Char('/'), size, QStringLiteral("/apps"), m_nativeIcons);
    if(!apkRoot.isEmpty())
        loadIcons(apkRoot + QLatin1Char('/'), size, QString(), m_apkIcons);

    const QString dynclockRoot = IconPaths::resolvePackCapabilityDir(m_path, QStringLiteral("dynclock"));
    if(!dynclockRoot.isEmpty())
    {
        QHash<QString, QString> clockIcons;
        loadIcons(dynclockRoot + QLatin1Char('/'), size, QString(), clockIcons);

        m_clockDial = clockIcons.value(QStringLiteral("bg"));
        m_hoursHand = clockIcons.value(QStringLiteral("hour"));
        m_minutesHand = clockIcons.value(QStringLiteral("minute"));

        const bool hasDyn = !m_clockDial.isEmpty() && !m_hoursHand.isEmpty() && !m_minutesHand.isEmpty();
        setHasDynamicClockIcon(hasDyn);
        if(hasDyn)
        {
            m_clockDial = QStringLiteral("dynclock/") + m_clockDial + QStringLiteral("/bg.png");
            m_hoursHand = QStringLiteral("dynclock/") + m_hoursHand + QStringLiteral("/hour.png");
            m_minutesHand = QStringLiteral("dynclock/") + m_minutesHand + QStringLiteral("/minute.png");
        }
    }

    const QString dyncalRoot = IconPaths::resolvePackCapabilityDir(m_path, QStringLiteral("dyncal"));
    if(!dyncalRoot.isEmpty())
    {
        QHash<QString, QString> calendarIcons;
        loadIcons(dyncalRoot + QLatin1Char('/'), size, QString(), calendarIcons);

        const bool hasDyn = calendarIcons.size() >= 31;
        setHasDynamicCalendarIcon(hasDyn);
        if(hasDyn)
        {
            for(int i = 0; i < 31; ++i)
            {
                const QString day = QString::number(i + 1).rightJustified(2, QLatin1Char('0'));
                const QString daySize = calendarIcons.value(day);
                m_calendarIcons[i] = QStringLiteral("dyncal/") + daySize + QLatin1Char('/') + day + QStringLiteral(".png");
            }
        }
    }
}

QStringList HarbourThemePack::icons()
{
    if(!m_icons.isEmpty())
        return m_icons;

    for(const QString& iconName : m_jollaIcons.keys())
        m_icons += findJollaIcon(iconName);
    for(const QString& iconName : m_nativeIcons.keys())
        m_icons += findNativeIcon(iconName);
    for(const QString& iconName : m_apkIcons.keys())
        m_icons += findApkIcon(iconName);

    return m_icons;
}

QString HarbourThemePack::iconByPackageName(const QString& packageName)
{
    static const QHash<QString, QString> jollaPackages = {
        {QStringLiteral("fingerterm"), QStringLiteral("icon-launcher-shell")},
        {QStringLiteral("jolla-calculator"), QStringLiteral("icon-launcher-calculator")},
        {QStringLiteral("jolla-calendar"), QStringLiteral("icon-launcher-calendar")},
        {QStringLiteral("jolla-camera"), QStringLiteral("icon-launcher-camera")},
        {QStringLiteral("jolla-clock"), QStringLiteral("icon-launcher-clock")},
        {QStringLiteral("jolla-contacts"), QStringLiteral("icon-launcher-people")},
        {QStringLiteral("jolla-email"), QStringLiteral("icon-launcher-email")},
        {QStringLiteral("jolla-gallery"), QStringLiteral("icon-launcher-gallery")},
        {QStringLiteral("jolla-mediaplayer"), QStringLiteral("icon-launcher-mediaplayer")},
        {QStringLiteral("jolla-messages"), QStringLiteral("icon-launcher-messaging")},
        {QStringLiteral("jolla-notes"), QStringLiteral("icon-launcher-notes")},
        {QStringLiteral("jolla-settings"), QStringLiteral("icon-launcher-settings")},
        {QStringLiteral("sailfish-browser"), QStringLiteral("icon-launcher-browser")},
        {QStringLiteral("sailfish-filemanager"), QStringLiteral("icon-launcher-file-manager")},
        {QStringLiteral("sailfish-audiorecorder"), QStringLiteral("icon-launcher-audiorecorder")},
        {QStringLiteral("sailfish-office"), QStringLiteral("icon-launcher-office")},
        {QStringLiteral("sailfish-tutorial"), QStringLiteral("icon-launcher-tutorial")},
        {QStringLiteral("sailfish-weather"), QStringLiteral("icon-launcher-weather")},
        {QStringLiteral("store-client"), QStringLiteral("icon-launcher-store")},
        {QStringLiteral("voicecall-ui"), QStringLiteral("icon-launcher-phone")},
    };

    if(jollaPackages.contains(packageName))
        return findJollaIcon(jollaPackages.value(packageName));

    return findNativeIcon(packageName);
}

QString HarbourThemePack::iconByActivity(const QString& activity)
{
    const QString normalized = QString(activity).replace(QLatin1Char('.'), QLatin1Char('_')).replace(QLatin1Char('/'), QLatin1Char('-'));
    return findApkIcon(QStringLiteral("apkd_launcher_") + normalized);
}

QImage HarbourThemePack::requestIcon(const QString& iconId, const QSize& requestedSize)
{
    if(iconId.isEmpty())
        return {};

    // Prefer packed relative paths (jolla/.../icons/foo.png) from icons()/find*.
    if(iconId.contains(QLatin1Char('/')))
        return loadImageFromFile(m_path + QLatin1Char('/') + iconId, requestedSize);

    // Bare icon name from per-app / legacy URIs.
    QString relative = findJollaIcon(iconId);
    if(relative.isEmpty())
        relative = findNativeIcon(iconId);
    if(relative.isEmpty())
        relative = findApkIcon(iconId);
    if(relative.isEmpty())
        return loadImageFromFile(m_path + QLatin1Char('/') + iconId, requestedSize);
    return loadImageFromFile(m_path + QLatin1Char('/') + relative, requestedSize);
}

QImage HarbourThemePack::requestClockDialIcon(const QSize& requestedSize)
{
    if(!hasDynamicClockIcon())
        return requestIcon(findJollaIcon(QStringLiteral("icon-launcher-clock")), requestedSize);
    return requestIcon(m_clockDial, requestedSize);
}

QImage HarbourThemePack::requestHoursHandIcon(const QSize& requestedSize)
{
    if(!hasDynamicClockIcon())
        return {};
    return requestIcon(m_hoursHand, requestedSize);
}

QImage HarbourThemePack::requestMinutesHandIcon(const QSize& requestedSize)
{
    if(!hasDynamicClockIcon())
        return {};
    return requestIcon(m_minutesHand, requestedSize);
}

QImage HarbourThemePack::requestCalendarIcon(const QDate& date, const QSize& requestedSize)
{
    if(!hasDynamicCalendarIcon() || !date.isValid())
        return requestIcon(findJollaIcon(QStringLiteral("icon-launcher-calendar")), requestedSize);
    return requestIcon(m_calendarIcons[date.day() - 1], requestedSize);
}

IconPack* HarbourThemePack::byShortName(const QString& shortName)
{
    if(shortName.isEmpty() || shortName == QLatin1String("default"))
        return nullptr;

    const QString full = IconPaths::packDir(shortName);
    if(!QDir(full).exists())
        return nullptr;

    return new HarbourThemePack(full);
}

QString HarbourThemePack::findJollaIcon(const QString& iconName)
{
    const QString scale = m_jollaIcons.value(iconName);
    if(scale.isEmpty())
        return QString();
    return QStringLiteral("jolla/") + scale + QStringLiteral("/icons/") + iconName + QStringLiteral(".png");
}

QString HarbourThemePack::findNativeIcon(const QString& iconName)
{
    const QString sz = m_nativeIcons.value(iconName);
    if(sz.isEmpty())
        return QString();
    return QStringLiteral("native/") + sz + QStringLiteral("/apps/") + iconName + QStringLiteral(".png");
}

QString HarbourThemePack::findApkIcon(const QString& iconName)
{
    const QString sz = m_apkIcons.value(iconName);
    if(sz.isEmpty())
        return QString();
    return QStringLiteral("apk/") + sz + QLatin1Char('/') + iconName + QStringLiteral(".png");
}
