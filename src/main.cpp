// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Core.h"
#include "QmlTypes.h"
#include "Translations.h"

#include <QGuiApplication>
#include <QLocale>
#include <QQuickView>
#include <QScopedPointer>
#include <sailfishapp.h>

// Exported so the Silica booster can dlopen() the binary and call main().
Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QGuiApplication::setApplicationVersion(QStringLiteral(SALAMA_VERSION));

    Salama::installTranslations(app.data(), QLocale(),
                                SailfishApp::pathTo(QStringLiteral("translations")).toLocalFile());

    Salama::Core core(Salama::Storage::defaultDataDirectory(),
                      Salama::Storage::defaultConfigFilePath(),
                      Salama::Storage::defaultDownloadDirectory());
    Salama::registerQmlTypes(&core);
    // Before the window is loaded: a share that started the browser is a call waiting
    // for the name to be claimed, and the share sheet does not wait long. The window
    // says when it is ready for what the call brought (qml/harbour-salama.qml).
    core.shareReceiver()->registerService();
    QObject::connect(app.data(), &QGuiApplication::aboutToQuit, &core, &Salama::Core::clearOnClose);
    // A page's notifications go with the page, and every page goes with the browser.
    QObject::connect(app.data(), &QGuiApplication::aboutToQuit, core.webNotifications(),
                     &Salama::WebNotifications::closeAll);

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    return QGuiApplication::exec();
}
