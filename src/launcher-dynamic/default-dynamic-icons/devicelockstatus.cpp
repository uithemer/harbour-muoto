#include "devicelockstatus.h"

DevicelockStatus::DevicelockStatus(QObject* parent)
    : QObject(parent)
    , m_mceInterface(QStringLiteral("com.nokia.mce"),
                     QStringLiteral("/com/nokia/mce/signal"),
                     QStringLiteral("com.nokia.mce.signal"),
                     QDBusConnection::systemBus())
{
    connect(&m_mceInterface, SIGNAL(tklock_mode_ind(QString)),
            this, SLOT(tklockModeChanged(QString)));
}

void DevicelockStatus::tklockModeChanged(const QString& mode)
{
    if(mode == QStringLiteral("unlocked"))
        emit unlocked();
    else if(mode == QStringLiteral("locked"))
        emit locked();
}
