#include "iconmanifest.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

IconManifest::IconManifest(const QString& path)
    : _path(path), _version(1)
{
}

bool IconManifest::load()
{
    _entries.clear();
    _activeIconPack.clear();
    _version = 1;

    QFile f(_path);
    if(!f.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();

    if(err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    QJsonObject root = doc.object();
    _version = root.value(QStringLiteral("version")).toInt(1);

    const QJsonValue aip = root.value(QStringLiteral("active_icon_pack"));
    if(aip.isString())
        _activeIconPack = aip.toString();

    const QJsonObject entries = root.value(QStringLiteral("entries")).toObject();
    for(auto it = entries.begin(); it != entries.end(); ++it)
    {
        const QJsonObject o = it.value().toObject();
        Entry e;
        e.originalIcon = o.value(QStringLiteral("original_icon")).toString();
        e.themedIcon = o.value(QStringLiteral("themed_icon")).toString();
        e.kind = o.value(QStringLiteral("kind")).toString();
        _entries.insert(it.key(), e);
    }

    return true;
}

bool IconManifest::save() const
{
    QJsonObject entries;
    for(auto it = _entries.begin(); it != _entries.end(); ++it)
    {
        QJsonObject o;
        o.insert(QStringLiteral("original_icon"), it.value().originalIcon);
        o.insert(QStringLiteral("themed_icon"), it.value().themedIcon);
        o.insert(QStringLiteral("kind"), it.value().kind);
        entries.insert(it.key(), o);
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), _version);

    if(_activeIconPack.isEmpty())
        root.insert(QStringLiteral("active_icon_pack"), QJsonValue(QJsonValue::Null));
    else
        root.insert(QStringLiteral("active_icon_pack"), _activeIconPack);

    root.insert(QStringLiteral("entries"), entries);

    QJsonDocument doc(root);

    QFileInfo fi(_path);
    QDir().mkpath(fi.absolutePath());

    QSaveFile sf(_path);
    if(!sf.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    sf.write(doc.toJson(QJsonDocument::Indented));
    return sf.commit();
}
