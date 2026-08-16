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

    // False once something else has written the file we themed inplace, which is
    // what an rpm update of the owning app does to a hicolor icon.
    static bool isThemedIconIntact(const QString& iconPath);

    // Whether the most recent update() actually wrote the icon. The constructor
    // performs one, so this is meaningful straight after construction. Apply
    // reports a real result from these rather than assuming success.
    bool lastUpdateOk() const;

public slots:
    void update();

private:
    QScopedPointer<IconUpdaterPrivate> d_ptr;
};

#endif // ICONUPDATER_H
