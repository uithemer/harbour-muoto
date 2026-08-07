#include "iconpack.h"
#include "iconpack_p.h"
#include "iconprovider.h"
#include "iconupdater.h"
#include "svgiconrender.h"

#include <MDesktopEntry>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QImage>

namespace {

class IconPackIconProvider : public IconProvider
{
    Q_OBJECT

public:
    IconPackIconProvider(IconPack* iconPack, const QString& iconId)
        : IconProvider(iconPack)
        , m_iconPack(iconPack)
        , m_iconId(iconId)
    {
    }

    QImage requestImage(const QSize& requestedSize) override
    {
        return m_iconPack->requestIcon(m_iconId, requestedSize);
    }

private:
    IconPack* m_iconPack;
    QString m_iconId;
};

} // namespace

IconPackPrivate::IconPackPrivate(const QString& name)
    : name(name)
{
}

IconPack::IconPack(const QString& name, QObject* parent)
    : QObject(parent)
    , d_ptr(new IconPackPrivate(name))
{
}

IconPack::~IconPack() = default;

QString IconPack::name() const
{
    return d_ptr->name;
}

bool IconPack::hasDynamicClockIcon() const
{
    return d_ptr->hasDynamicClockIcon;
}

bool IconPack::hasDynamicCalendarIcon() const
{
    return d_ptr->hasDynamicCalendarIcon;
}

void IconPack::setHasDynamicClockIcon(bool v)
{
    d_ptr->hasDynamicClockIcon = v;
}

void IconPack::setHasDynamicCalendarIcon(bool v)
{
    d_ptr->hasDynamicCalendarIcon = v;
}

QString IconPack::iconByDesktopPath(const QString& desktopPath)
{
    const QFileInfo info(desktopPath);
    const QString desktopBaseName = info.completeBaseName();

    if(desktopBaseName.startsWith(QLatin1String("apkd_launcher")))
    {
        MDesktopEntry desktopEntry(desktopPath);
        const QString activityPath = desktopEntry.exec().section(QLatin1Char(' '), -1);
        return iconByActivity(activityPath);
    }

    return iconByPackageName(desktopBaseName);
}

IconUpdater* IconPack::iconUpdater(const QString& desktopPath, const QString& iconId)
{
    auto* provider = new IconPackIconProvider(this, iconId);
    return new IconUpdater(provider, desktopPath, this);
}

QImage IconPack::loadImageFromFile(const QString& path, const QSize& requestedSize)
{
    if(!QFile::exists(path))
    {
        qWarning() << "muoto-launcher: file not found" << path;
        return {};
    }

    if(path.endsWith(QStringLiteral(".svg")))
    {
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly))
            return {};
        return renderSvgIcon(file.readAll(), requestedSize);
    }

    QImage img(path);
    if(img.isNull() || !requestedSize.isValid())
        return img;

    return img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

#include "iconpack.moc"
