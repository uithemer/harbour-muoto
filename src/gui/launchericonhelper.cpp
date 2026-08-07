#include "launchericonhelper.h"
#include "harbourthemepack.h"
#include "iconpack.h"

LauncherIconHelper::LauncherIconHelper(QObject* parent)
    : QObject(parent)
    , m_iconPack(nullptr)
{
}

QString LauncherIconHelper::iconPackName() const
{
    return m_iconPackName;
}

void LauncherIconHelper::setIconPackName(const QString& name)
{
    if(m_iconPackName == name)
        return;

    m_iconPackName = name;
    delete m_iconPack;
    m_iconPack = name.isEmpty() ? nullptr : HarbourThemePack::byShortName(name);
    emit iconPackNameChanged();
}

QString LauncherIconHelper::iconSource(const QString& desktopPath) const
{
    if(!m_iconPack)
        return QString();

    const QString id = m_iconPack->iconByDesktopPath(desktopPath);
    if(id.isEmpty())
        return QString();

    return QStringLiteral("image://muoto-launcher/icon-pack/%1/%2")
        .arg(m_iconPackName, id);
}

QStringList LauncherIconHelper::packIconIds() const
{
    if(!m_iconPack)
        return QStringList();
    return m_iconPack->icons();
}
