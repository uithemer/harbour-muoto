#include "desktopentry.h"

#include <MDesktopEntry>
#include <QDebug>
#include <QFile>

DesktopEntry::DesktopEntry(const QString& path)
    : m_path(path)
    , m_hasChanges(false)
    , m_keyFile(g_key_file_new())
    , m_desktopEntry(new MDesktopEntry(path))
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        return;

    g_autoptr(GError) error = nullptr;
    const QByteArray content = file.readAll();
    const gboolean loaded = g_key_file_load_from_data(m_keyFile,
                                                      content.constData(),
                                                      static_cast<gsize>(content.length()),
                                                      G_KEY_FILE_KEEP_TRANSLATIONS,
                                                      &error);
    if(!loaded)
        qWarning() << "muoto-launcher: could not load desktop file" << path << ":" << error->message;
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
    const QByteArray iconBa = icon.toUtf8();
    g_key_file_set_string(m_keyFile, "Desktop Entry", "Icon", iconBa.constData());
    m_hasChanges = true;
}

bool DesktopEntry::save()
{
    if(!m_hasChanges)
        return true;

    gsize length = 0;
    g_autoptr(GError) error = nullptr;
    g_autofree char* content = g_key_file_to_data(m_keyFile, &length, &error);
    if(content == nullptr)
    {
        qWarning() << "muoto-launcher: could not serialize desktop file" << m_path << ":" << error->message;
        return false;
    }

    QFile file(m_path);
    if(!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "muoto-launcher: could not open for write" << m_path << ":" << file.errorString();
        return false;
    }

    const QByteArray contentBa(content, static_cast<int>(length));
    if(file.write(contentBa) != contentBa.size())
    {
        qWarning() << "muoto-launcher: could not write desktop file" << m_path;
        return false;
    }

    file.close();
    m_hasChanges = false;
    return true;
}
