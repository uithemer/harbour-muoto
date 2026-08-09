#include "overlayrender.h"
#include "iconpaths.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QPainter>
#include <QFileInfo>

namespace OverlayRender {

namespace {

QStringList overlayPngs(const QString& packRoot)
{
    const QString overlayDir = IconPaths::resolvePackCapabilityDir(packRoot,
                                                                 QStringLiteral("overlay"));
    if(overlayDir.isEmpty())
        return QStringList();

    QStringList all;
    QDirIterator it(overlayDir, QStringList() << QStringLiteral("*.png"),
                    QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
        all << it.next();
    return all;
}

} // namespace

QImage composite(const QImage& overlayBase, const QImage& innerIcon,
                 const QSize& outerSize, const QSize& innerSize)
{
    QImage out(outerSize, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);

    if(!overlayBase.isNull())
    {
        const QImage scaled = overlayBase.scaled(outerSize,
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::SmoothTransformation);
        p.drawImage(QPoint(0, 0), scaled);
    }

    if(!innerIcon.isNull())
    {
        const QImage scaledInner = innerIcon.scaled(innerSize,
                                                    Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation);
        const int x = (outerSize.width()  - scaledInner.width())  / 2;
        const int y = (outerSize.height() - scaledInner.height()) / 2;
        p.drawImage(QPoint(x, y), scaledInner);
    }

    p.end();
    return out;
}

QString overlayBaseForDesktop(const QString& packRoot, const QString& desktopPath)
{
    const QStringList all = overlayPngs(packRoot);
    if(all.isEmpty())
        return QString();

    const QString key = QFileInfo(desktopPath).completeBaseName();
    const uint idx = qHash(key) % static_cast<uint>(all.size());
    return all.at(static_cast<int>(idx));
}

} // namespace OverlayRender
