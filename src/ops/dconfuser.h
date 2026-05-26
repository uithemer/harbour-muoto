#ifndef DCONFUSER_H
#define DCONFUSER_H

#include <QString>
#include <QStringList>

// Run `dconf` with defaultuser's HOME / XDG_RUNTIME_DIR regardless of
// caller euid (GUI or helperd). All C++ read/write/reset of per-user
// dconf keys must use this — never invoke `dconf` directly for user keys.
bool runDconfAsDefaultUser(const QStringList& args, QString* stdOut = nullptr);

#endif
