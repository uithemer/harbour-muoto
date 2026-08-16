#ifndef OPSTATUS_H
#define OPSTATUS_H

#include "muotolauncherglobal.h"
#include <QString>

// Last icon-operation outcome, on disk next to launcher-manifest.json.
//
// Two consumers need it. The shell callers (harbour-muoto-update-icons, the
// repair oneshot) previously inferred success from the icon-ops.lock lifecycle,
// which is why a rejected apply could still be logged as a success; reading a
// signal from a script means dbus-monitor, which is too fragile to gate the
// repair on. And journald is Storage=volatile on device, so a user bug report
// otherwise arrives with no history at all.
//
// The sequence is seeded from the file at startup, never from zero: the repair
// restarts the daemon and then immediately runs update-icons, so a counter that
// rewound would make the caller misread whose result it is reading.
namespace OpStatus {

enum Outcome
{
    Ok = 0,
    Partial,     // some updaters were refused; the pack is still applied
    HardFailure  // nothing was written
};

MUOTO_LAUNCHER_EXPORT QString path();

// Highest sequence seen in the file, or 0. Call once at startup.
MUOTO_LAUNCHER_EXPORT quint64 seedSequence();

MUOTO_LAUNCHER_EXPORT void record(const QString& op, Outcome outcome, const QString& message,
                                  int built, int written);

} // namespace OpStatus

#endif // OPSTATUS_H
