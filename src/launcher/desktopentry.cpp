#include "desktopentry.h"

#include <MDesktopEntry>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

namespace {

const char kGroup[] = "Desktop Entry";

bool hasRequiredKeys(GKeyFile* keyFile)
{
    if(g_key_file_has_key(keyFile, kGroup, "Type", nullptr) == FALSE)
        return false;

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

DesktopEntry::DesktopEntry(const QString& path)
    : m_path(path)
    , m_loaded(false)
    , m_hasChanges(false)
    , m_keyFile(g_key_file_new())
    , m_desktopEntry(new MDesktopEntry(path))
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
    const gboolean ok = g_key_file_load_from_data(m_keyFile,
                                                  content.constData(),
                                                  static_cast<gsize>(content.length()),
                                                  G_KEY_FILE_KEEP_TRANSLATIONS,
                                                  &error);
    if(ok == FALSE)
    {
        qWarning() << "muoto-launcher: could not load desktop file" << path << ":"
                   << (error ? error->message : "unknown error");
        return;
    }

    m_loaded = true;
}

DesktopEntry::~DesktopEntry()
{
    g_key_file_free(m_keyFile);
}

QString DesktopEntry::icon()
{
    return m_desktopEntry->icon();
}

void DesktopEntry::setIcon(const QString& icon)
{
    if(!m_loaded)
        return;

    const QByteArray iconBa = icon.toUtf8();
    g_key_file_set_string(m_keyFile, kGroup, "Icon", iconBa.constData());
    m_hasChanges = true;
}

bool DesktopEntry::save()
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
    g_autofree char* content = g_key_file_to_data(m_keyFile, &length, &error);
    if(content == nullptr)
    {
        qWarning() << "muoto-launcher: could not serialize desktop file" << m_path << ":"
                   << (error ? error->message : "unknown error");
        return false;
    }

    const QByteArray contentBa(content, static_cast<int>(length));
    if(contentBa.isEmpty())
    {
        qWarning() << "muoto-launcher: refusing to write empty desktop file" << m_path;
        return false;
    }

    QFile file(m_path);
    if(!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "muoto-launcher: could not open for write" << m_path << ":"
                   << file.errorString();
        return false;
    }

    if(file.write(contentBa) != contentBa.size())
    {
        qWarning() << "muoto-launcher: could not write desktop file" << m_path;
        return false;
    }

    file.close();
    m_hasChanges = false;
    return true;
}
