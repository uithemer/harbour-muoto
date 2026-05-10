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
#include <QQmlEngine>
#include <QFileSystemWatcher>
#include "themepack.h"
#include "themepackmodel.h"
#include "fontweightmodel.h"
#include "iconapplier.h"
#include "iconpreviewprovider.h"

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

    // image://uithemer/preview/<packName>?t=<ts> serves the in-memory icon
    // pack preview built by IconApplier::buildPreview. Engine takes ownership.
    view->engine()->addImageProvider(QStringLiteral("uithemer"),
                                     new IconPreviewProvider);

    view->setSource(SailfishApp::pathTo(qml));
    view->show();
    return app->exec();
}
