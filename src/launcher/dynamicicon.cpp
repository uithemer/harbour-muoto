#include "dynamicicon.h"
#include "dynamicicon_p.h"
#include "iconupdater.h"
#include "launcherpaths.h"

#include <MGConfItem>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QMap>
#include <QUrl>

namespace {

QList<QMetaObject> dynamicIconsMeta;

void loadDynamicIconPlugins()
{
    QDir pluginsDir(LauncherPaths::dynamicIconsPluginDir());
    const QFileInfoList pluginsInfo = pluginsDir.entryInfoList({QStringLiteral("*.so")}, QDir::Files);

    for(const QFileInfo& pluginInfo : pluginsInfo)
    {
        QLibrary library(pluginInfo.absoluteFilePath());
        if(library.load())
            qDebug() << "muoto-launcher: loaded plugin" << pluginInfo.fileName();
        else
            qWarning() << "muoto-launcher: could not load" << pluginInfo.fileName() << library.errorString();
    }
}

} // namespace

DynamicIconPrivate::DynamicIconPrivate(const QString& packageName,
                                       const QString& name,
                                       DynamicIcon* parent)
    : QObject(parent)
    , iconProvider(nullptr)
    , name(name)
    , packageName(packageName)
{
    desktopPath = QStringLiteral("/usr/share/applications/%1.desktop").arg(packageName);

    applicationProvider = new MGConfItem(LauncherPaths::perAppProviderKey(packageName), this);

    const QString none = QStringLiteral("<none>");
    if(applicationProvider->value(none).toString() == none)
        applicationProvider->set(QStringLiteral("dynamic-icon://") + name);

    connect(applicationProvider, &MGConfItem::valueChanged, parent, &DynamicIcon::enabledChanged);
}

DynamicIcon::DynamicIcon(const QString& packageName, const QString& name, QObject* parent)
    : QObject(parent)
    , d_ptr(new DynamicIconPrivate(packageName, name, this))
{
}

QString DynamicIcon::name()
{
    return d_ptr->name;
}

bool DynamicIcon::available()
{
    return QFile::exists(d_ptr->desktopPath);
}

bool DynamicIcon::enabled()
{
    const QUrl uri(d_ptr->applicationProvider->value().toString());
    return uri.scheme() == QStringLiteral("dynamic-icon") && uri.host() == name();
}

IconProvider* DynamicIcon::iconProvider()
{
    if(d_ptr->iconProvider == nullptr)
        d_ptr->iconProvider = createIconProvider(this);
    return d_ptr->iconProvider;
}

IconUpdater* DynamicIcon::iconUpdater()
{
    return new IconUpdater(iconProvider(), d_ptr->desktopPath, this);
}

void registerDynamicIconMeta(const QMetaObject& meta)
{
    dynamicIconsMeta.append(meta);
}

QList<DynamicIcon*> loadDynamicIcons()
{
    static QMap<QString, DynamicIcon*> dynamicIcons;

    if(!dynamicIcons.isEmpty())
        return dynamicIcons.values();

    loadDynamicIconPlugins();

    for(const QMetaObject& meta : dynamicIconsMeta)
    {
        auto* dynamicIcon = qobject_cast<DynamicIcon*>(meta.newInstance());
        if(!dynamicIcon)
        {
            qWarning() << "muoto-launcher: could not instantiate" << meta.className();
            continue;
        }

        const QString iconName = dynamicIcon->name();
        if(dynamicIcons.contains(iconName))
        {
            delete dynamicIcon;
            continue;
        }

        dynamicIcons.insert(iconName, dynamicIcon);
        qDebug() << "muoto-launcher: registered dynamic icon" << meta.className();
    }

    return dynamicIcons.values();
}
