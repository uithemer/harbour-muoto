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
#include "iconapplier.h"
#include "fontapplier.h"
#include "helperclient.h"
#include "iconpreviewprovider.h"

// 2.6.0: setuid(0) is gone from main(). The GUI runs as defaultuser
// (its packaged .desktop file targets the user session) and gets
// privileges only when needed via HelperClient -> system bus ->
// sailfishos-uithemer-helperd, which runs as root and is gated by 3
// polkit actions (manage-themes, manage-packs,
// manage-system-services, all auth_admin_keep so the user is prompted
// once per ~5 minutes).
//
// IconApplier stays a QML type so ConfirmPage can call buildPreview()
// directly (read-only, in-process). DensityEnabler is no longer
// registered: the only QML caller (DensityPage) was the
// ensureEnabled() path, which now goes through Helper.densityEnable().
int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();
    QString qml = QString("qml/sailfishos-uithemer.qml");

    qmlRegisterType<ThemePack>("harbour.uithemer", 1, 0, "ThemePack");
    qmlRegisterType<ThemePackModel>("harbour.uithemer", 1, 0, "ThemePackModel");
    qmlRegisterType<FontWeightModel>("harbour.uithemer", 1, 0, "FontWeightModel");
    qmlRegisterType<IconApplier>("harbour.uithemer", 1, 0, "IconApplier");
    qmlRegisterType<FontApplier>("harbour.uithemer", 1, 0, "FontApplier");
    qmlRegisterSingletonType<HelperClient>("harbour.uithemer", 1, 0, "Helper",
                                           &HelperClient::qmlSingleton);

    // image://uithemer/preview/<packName>?t=<ts> serves the in-memory
    // icon pack preview built by IconApplier::buildPreview. Engine
    // takes ownership.
    view->engine()->addImageProvider(QStringLiteral("uithemer"),
                                     new IconPreviewProvider);

    view->setSource(SailfishApp::pathTo(qml));
    view->show();
    return app->exec();
}
