#ifndef ICONUPDATER_H
#define ICONUPDATER_H

#include "muotolauncherglobal.h"
#include <QObject>
#include <QScopedPointer>
#include <QString>

class IconProvider;

class IconUpdaterPrivate;

class MUOTO_LAUNCHER_EXPORT IconUpdater : public QObject
{
    Q_OBJECT

public:
    enum Mode
    {
        Default = 0,
        RedirectOnly
    };

    IconUpdater(IconProvider* provider, const QString& desktopPath,
                QObject* parent = nullptr, Mode mode = Default);
    ~IconUpdater() override;

public slots:
    void update();

private:
    QScopedPointer<IconUpdaterPrivate> d_ptr;
};

#endif // ICONUPDATER_H
