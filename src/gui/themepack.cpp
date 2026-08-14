#include "themepack.h"

#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QFile>
#include <QStandardPaths>
#include <QRegularExpression>

ThemePack::ThemePack(QObject* parent)
    : QObject(parent)
{
}

bool ThemePack::hasStoremanInstalled() const
{
    return QFileInfo("/usr/share/harbour-storeman/qml/harbour-storeman.qml").exists();
}

void ThemePack::restartHomescreen()
{
    // GUI runs as defaultuser — user-bus systemctl --user talks to this
    // session's systemd. startDetached and forget.
    QProcess::startDetached(QStringLiteral("systemctl"),
        QStringList() << QStringLiteral("--user")
                      << QStringLiteral("restart")
                      << QStringLiteral("lipstick.service"));
}

QString ThemePack::activeFontWeightBasename() const
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                       + QStringLiteral("/fontconfig/conf.d/99-muoto.conf");
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly))
        return QString();

    static const QRegularExpression re(QStringLiteral("weight '([^']+)'"));
    const QRegularExpressionMatch m = re.match(QString::fromUtf8(f.readAll()));
    if(!m.hasMatch())
        return QString();
    return m.captured(1);
}
