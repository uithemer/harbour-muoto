#ifndef SPAWNER_H
#define SPAWNER_H

#include <QString>

// 2.6.0: setuid_ex was a no-return-value wrapper for callers that did
// `setuid(0)` to elevate the GUI process to root before spawning a
// privileged shell. With the GUI now running as defaultuser and every
// privileged op going through harbour-muoto-helperd over D-Bus,
// no in-process call site needs root anymore.
namespace Spawner
{
    QString executeSync(const QString& cmd);
}

#endif // SPAWNER_H
