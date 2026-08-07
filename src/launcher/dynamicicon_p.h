#ifndef DYNAMICICON_P_H
#define DYNAMICICON_P_H

#include <QObject>
#include <QString>

class DynamicIcon;
class IconProvider;
class MGConfItem;

class DynamicIconPrivate : public QObject
{
    Q_OBJECT

public:
    DynamicIconPrivate(const QString& packageName, const QString& name, DynamicIcon* parent);

    IconProvider* iconProvider;
    QString name;
    QString packageName;
    QString displayName;
    QString desktopPath;
    MGConfItem* applicationProvider;
};

#endif // DYNAMICICON_P_H
