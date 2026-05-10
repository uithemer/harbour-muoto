#ifndef AUTH_H
#define AUTH_H

#include <QString>
#include <QDBusMessage>

// Tiny wrapper around PolkitQt1::Authority::checkAuthorizationSync so
// individual adaptor slots stay readable. AllowUserInteraction is
// always passed: lipstick's polkit auth agent prompts the user, and
// auth_admin_keep cached results bypass the prompt for ~5 minutes.
//
// On any failure (polkit binary missing on community port, agent not
// running, user cancels the prompt) returns false and lets the caller
// emit OperationCompleted(op, false, "polkit denied") so the GUI's
// busy spinner clears. Never throws.
namespace UiThemerAuth
{
    bool isAuthorized(const QDBusMessage& message, const QString& actionId);
}

#endif // AUTH_H
