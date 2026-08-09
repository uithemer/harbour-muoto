#ifndef ICONPACK_P_H
#define ICONPACK_P_H

#include <QString>

class IconPackPrivate
{
public:
    explicit IconPackPrivate(const QString& name);

    QString name;
    bool hasDynamicClockIcon = false;
    bool hasDynamicCalendarIcon = false;
};

#endif // ICONPACK_P_H
