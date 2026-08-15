#ifndef ICONPACK_H
#define ICONPACK_H

#include "muotolauncherglobal.h"
#include <QDate>
#include <QImage>
#include <QObject>
#include <QScopedPointer>
#include <QSize>
#include <QString>

class IconUpdater;

class IconPackPrivate;

class MUOTO_LAUNCHER_EXPORT IconPack : public QObject
{
    Q_OBJECT

public:
    explicit IconPack(const QString& name, QObject* parent = nullptr);
    ~IconPack() override;

    QString name() const;

    virtual QString iconByPackageName(const QString& packageName) = 0;
    virtual QString iconByActivity(const QString& activity) = 0;
    virtual QImage requestIcon(const QString& iconId, const QSize& requestedSize) = 0;
    virtual QImage requestClockDialIcon(const QSize& requestedSize) = 0;
    virtual QImage requestHoursHandIcon(const QSize& requestedSize) = 0;
    virtual QImage requestMinutesHandIcon(const QSize& requestedSize) = 0;
    virtual QImage requestCalendarIcon(const QDate& date, const QSize& requestedSize) = 0;

    bool hasDynamicClockIcon() const;
    bool hasDynamicCalendarIcon() const;

    QString iconByDesktopPath(const QString& desktopPath);
    IconUpdater* iconUpdater(const QString& desktopPath, const QString& iconId);

protected:
    void setHasDynamicClockIcon(bool v);
    void setHasDynamicCalendarIcon(bool v);
    static QImage loadImageFromFile(const QString& path, const QSize& requestedSize);

private:
    QScopedPointer<IconPackPrivate> d_ptr;
};

#endif // ICONPACK_H
