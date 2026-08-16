#include "desktopfile.h"
#include "filewrite.h"
#include "launcherpaths.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

const char kGroup[] = "Desktop Entry";

// A launcher entry Lipstick can build needs at least these two. Anything we are
// about to write that lacks them is a stub, whatever the reason.
bool hasRequiredKeys(GKeyFile* keyFile)
{
    if(g_key_file_has_key(keyFile, kGroup, "Type", nullptr) == FALSE)
        return false;

    // Name may only exist as a localised variant on some entries.
    if(g_key_file_has_key(keyFile, kGroup, "Name", nullptr) != FALSE)
        return true;

    gsize count = 0;
    g_autofree gchar** keys = g_key_file_get_keys(keyFile, kGroup, &count, nullptr);
    if(keys == nullptr)
        return false;

    for(gsize i = 0; i < count; ++i)
    {
        if(g_str_has_prefix(keys[i], "Name["))
            return true;
    }
    return false;
}

} // namespace

DesktopFile::DesktopFile(const QString& path)
    : m_path(path)
    , m_keyFile(g_key_file_new())
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "muoto-launcher: could not read desktop file" << path << ":"
                   << file.errorString();
        return;
    }

    g_autoptr(GError) error = nullptr;
    const QByteArray content = file.readAll();
    // KEEP_COMMENTS so a rewrite that only re-points Icon= stops stripping the
    // packaged file's comments (and stops showing up in rpm -V as a checksum
    // difference we caused for no reason).
    const gboolean ok = g_key_file_load_from_data(m_keyFile,
                                                  content.constData(),
                                                  static_cast<gsize>(content.length()),
                                                  GKeyFileFlags(G_KEY_FILE_KEEP_TRANSLATIONS
                                                                | G_KEY_FILE_KEEP_COMMENTS),
                                                  &error);
    if(ok == FALSE)
    {
        qWarning() << "muoto-launcher: could not parse desktop file" << path << ":"
                   << (error ? error->message : "unknown error");
        return;
    }

    m_original = content;
    m_loaded = true;
}

// Keeps a pristine copy the first time we touch an entry, so repairing a
// destroyed one later is a local copy rather than downloading its rpm. Only
// captures a desktop we have not already re-pointed, and never overwrites an
// existing capture.
void DesktopFile::captureBackupOnce() const
{
    const QString backupPath = LauncherPaths::desktopBackupPath(m_path);
    if(QFileInfo::exists(backupPath))
        return;

    if(m_original.isEmpty())
        return;

    if(!QFileInfo(backupPath).dir().mkpath(QStringLiteral(".")))
        return;

    QFile backup(backupPath);
    if(!backup.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    if(backup.write(m_original) != m_original.size())
    {
        backup.close();
        QFile::remove(backupPath);
    }
}

DesktopFile::~DesktopFile()
{
    g_key_file_free(m_keyFile);
}

QString DesktopFile::icon() const
{
    if(!m_loaded)
        return QString();

    g_autofree gchar* value = g_key_file_get_string(m_keyFile, kGroup, "Icon", nullptr);
    return value ? QString::fromUtf8(value) : QString();
}

void DesktopFile::setIcon(const QString& icon)
{
    if(!m_loaded)
        return;

    const QByteArray iconBa = icon.toUtf8();
    g_key_file_set_string(m_keyFile, kGroup, "Icon", iconBa.constData());
    m_hasChanges = true;
}

bool DesktopFile::save()
{
    if(!m_loaded)
    {
        qWarning() << "muoto-launcher: refusing to write unread desktop file" << m_path;
        return false;
    }

    if(!m_hasChanges)
        return true;

    if(!QFileInfo::exists(m_path))
    {
        qWarning() << "muoto-launcher: refusing to recreate vanished desktop file" << m_path;
        return false;
    }

    if(!hasRequiredKeys(m_keyFile))
    {
        qWarning() << "muoto-launcher: refusing to write desktop file without Type/Name"
                   << m_path;
        return false;
    }

    gsize length = 0;
    g_autoptr(GError) error = nullptr;
    g_autofree gchar* data = g_key_file_to_data(m_keyFile, &length, &error);
    if(data == nullptr)
    {
        qWarning() << "muoto-launcher: could not serialize desktop file" << m_path << ":"
                   << (error ? error->message : "unknown error");
        return false;
    }

    const QByteArray content(data, static_cast<int>(length));
    if(content.isEmpty())
    {
        qWarning() << "muoto-launcher: refusing to write empty desktop file" << m_path;
        return false;
    }

    captureBackupOnce();

    if(!FileWrite::inPlace(m_path, content))
        return false;

    m_hasChanges = false;
    return true;
}
