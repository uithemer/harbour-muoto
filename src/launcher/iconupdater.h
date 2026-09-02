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
        RedirectOnly,
        // Redirect to a filename that does not change between updates. For the
        // dynamic clock and calendar, which redraw every minute.
        RedirectStable
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

    // Puts the stock icon back and drops this entry's manifest record. Explicit:
    // the destructor no longer does it as a side effect.
    void restore();

public slots:
    void update();

    // What a provider's imageUpdated is wired to: asks the queue for a rebuild
    // rather than writing from whatever context the signal fired in.
    void requestUpdate();

private:
    QScopedPointer<IconUpdaterPrivate> d_ptr;
};

#endif // ICONUPDATER_H
