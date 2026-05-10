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
#include "iconapplier.h"

int main(int argc, char *argv[])
{
    qDebug() << setuid(0);

    QGuiApplication *app = SailfishApp::application(argc,argv);
    QQuickView *view = SailfishApp::createView();
    QString qml = QString("qml/sailfishos-uithemer.qml");

    qmlRegisterType<ThemePack>("harbour.uithemer", 1, 0, "ThemePack");
    qmlRegisterType<ThemePackModel>("harbour.uithemer", 1, 0, "ThemePackModel");
    qmlRegisterType<FontWeightModel>("harbour.uithemer", 1, 0, "FontWeightModel");
    qmlRegisterType<IconApplier>("harbour.uithemer", 1, 0, "IconApplier");

    view->setSource(SailfishApp::pathTo(qml));
    view->show();
    return app->exec();
}
