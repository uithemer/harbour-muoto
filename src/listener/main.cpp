#include "installlistener.h"

#include <QCoreApplication>
#include <QDBusMetaType>

int main(int argc, char* argv[])
{
    qDBusRegisterMetaType<QStringList>();

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("harbour-muoto-install-listener"));
    QCoreApplication::setOrganizationName(QStringLiteral("harbour-muoto"));

    InstallListener listener;
    Q_UNUSED(listener);

    return app.exec();
}
