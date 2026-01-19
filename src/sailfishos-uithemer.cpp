#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <sys/types.h>
#include <unistd.h>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QQuickView>
#include <QQmlContext>
#include <QGuiApplication>
#include <QFileSystemWatcher>
#include "themepack.h"
#include "themepackmodel.h"
#include "fontweightmodel.h"

int main(int argc, char *argv[])
{
    // Removed direct setuid(0) call. UI apps must not escalate privileges.
    // Privileged operations should be moved to a dedicated helper/service
    // and invoked securely (DBus + polkit or a packaged helper invoked via pkexec).
    // qDebug() << setuid(0);

    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();
    QString qml = QString("qml/sailfishos-uithemer.qml");

    qmlRegisterType<ThemePack>("harbour.uithemer", 1, 0, "ThemePack");
    qmlRegisterType<ThemePackModel>("harbour.uithemer", 1, 0, "ThemePackModel");
    qmlRegisterType<FontWeightModel>("harbour.uithemer", 1, 0, "FontWeightModel");

    view->setSource(SailfishApp::pathTo(qml));
    view->show();
    return app->exec();
}