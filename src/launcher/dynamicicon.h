#ifndef DYNAMICICON_H
#define DYNAMICICON_H

#include "muotolauncherglobal.h"
#include <QList>
#include <QObject>

class IconProvider;
class IconUpdater;
class DynamicIconPrivate;

class MUOTO_LAUNCHER_EXPORT DynamicIcon : public QObject
{
    Q_OBJECT

public:
    DynamicIcon(const QString& packageName, const QString& name, QObject* parent = nullptr);

    QString name();
    bool available();
    bool enabled();

    IconProvider* iconProvider();
    IconUpdater* iconUpdater();

protected:
    virtual IconProvider* createIconProvider(QObject* parent) = 0;

private:
    DynamicIconPrivate* d_ptr;
};

MUOTO_LAUNCHER_EXPORT
void registerDynamicIconMeta(const QMetaObject& meta);

MUOTO_LAUNCHER_EXPORT
QList<DynamicIcon*> loadDynamicIcons();

#define REGISTER_DYNAMIC_ICON(dynamicIcon) \
    static void __attribute__((__constructor__)) __register_##dynamicIcon(void) \
    { \
        registerDynamicIconMeta(dynamicIcon::staticMetaObject); \
    }

#endif // DYNAMICICON_H
