#include "auth.h"

#include <PolkitQt1/Authority>
#include <PolkitQt1/Subject>

#include <QDebug>

bool UiThemerAuth::isAuthorized(const QDBusMessage& message,
                                const QString& actionId)
{
    using PolkitQt1::Authority;
    using PolkitQt1::SystemBusNameSubject;

    Authority* authority = Authority::instance();
    if(!authority)
    {
        qWarning() << "uithemer-helperd: PolkitQt1 Authority unavailable";
        return false;
    }

    const QString caller = message.service();
    if(caller.isEmpty())
    {
        qWarning() << "uithemer-helperd: empty caller bus name; denying"
                   << actionId;
        return false;
    }

    SystemBusNameSubject subject(caller);
    const Authority::Result result = authority->checkAuthorizationSync(
        actionId, subject, Authority::AllowUserInteraction);

    if(authority->hasError())
    {
        qWarning() << "uithemer-helperd: polkit error checking" << actionId
                   << ":" << authority->errorDetails();
        authority->clearError();
        return false;
    }
    return result == Authority::Yes;
}
