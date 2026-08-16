#ifndef ICONBACKUP_H
#define ICONBACKUP_H

#include <QString>

// Stock-icon backup for inplace theming, in one place.
//
// There used to be two restore implementations with very different safety:
// iconupdater's restoreIcon() removed the live icon and then copied, with no
// check that a backup existed at all, so a missing blob meant the app simply
// lost its icon until its RPM was reinstalled. launchermanifest's
// restoreInplaceStock() did it properly. These are that second version.
namespace IconBackup {

bool exists(const QString& iconPath);

// Copies the live icon aside once. Returns false (and logs) if it could not,
// because a silently missing backup is what makes a later restore destructive.
bool create(const QString& iconPath);

// Writes the backup back into the live icon's existing inode, preserving uid,
// gid and mode -- the daemon has CAP_DAC_OVERRIDE but no CAP_CHOWN, so a
// remove-and-copy would leave a defaultuser-owned file it cannot chown back.
// Refuses a missing or empty backup rather than destroying the live icon.
bool restore(const QString& iconPath);

} // namespace IconBackup

#endif // ICONBACKUP_H
