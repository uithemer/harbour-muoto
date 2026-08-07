#ifndef FOLDERAMBIENT_H
#define FOLDERAMBIENT_H

#include <QString>

namespace FolderAmbient {

/**
 * Scoped silica writeback for icon-launcher-folder-01..16 under
 * sailfish-default/silica/<z>/icons/. Backups under backup/folder-icons/.
 */
void apply(const QString& packShortName, bool overlayEnabled);

/** Restore allowlisted folder icons from backup/folder-icons/ and remove backups. */
void restore();

} // namespace FolderAmbient

#endif // FOLDERAMBIENT_H
