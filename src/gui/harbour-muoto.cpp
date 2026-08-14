#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>

#include "themepack.h"
#include "themepackmodel.h"
#include "fontweightmodel.h"
#include "fontcarouselmodel.h"
#include "iconapplier.h"
#include "helperclient.h"
#include "iconpreviewprovider.h"
#include "fontsampleprovider.h"
#include "launcherimageprovider.h"

// 2.6.0: setuid(0) is gone from main(). The GUI runs as defaultuser
// (its packaged .desktop file targets the user session) and gets
// privileges only when needed via HelperClient -> system bus ->
// harbour-muoto-helperd (authorized by org.muoto.Muoto1.conf bus
// policy; no polkit).
//
// IconApplier stays a QML type so MainPage / configure pages can call
// buildPreview() directly (read-only, in-process). DensityEnabler is
// no longer registered: unlock goes through Helper.densityEnable().
int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();
    QString qml = QString("qml/harbour-muoto.qml");

    qmlRegisterType<ThemePack>("harbour.muoto", 1, 0, "ThemePack");
    qmlRegisterType<ThemePackModel>("harbour.muoto", 1, 0, "ThemePackModel");
    qmlRegisterType<FontWeightModel>("harbour.muoto", 1, 0, "FontWeightModel");
    qmlRegisterType<FontCarouselModel>("harbour.muoto", 1, 0, "FontCarouselModel");
    qmlRegisterType<IconApplier>("harbour.muoto", 1, 0, "IconApplier");
    qmlRegisterSingletonType<HelperClient>("harbour.muoto", 1, 0, "Helper",
                                           &HelperClient::qmlSingleton);

    // image://muoto/preview/<packName>?t=<ts> serves the in-memory
    // icon pack preview built by IconApplier::buildPreview. Engine
    // takes ownership.
    view->engine()->addImageProvider(QStringLiteral("muoto"),
                                     new IconPreviewProvider);
    view->engine()->addImageProvider(QStringLiteral("muoto-font"),
                                     new FontSampleProvider);
    view->engine()->addImageProvider(QStringLiteral("muoto-launcher"),
                                     new LauncherImageProvider);

    view->engine()->addImportPath(
        SailfishApp::pathTo(QStringLiteral("qml/modules")).toString());

    view->setSource(SailfishApp::pathTo(qml));
    view->show();
    return app->exec();
}
