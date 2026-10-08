// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Core.h"
#include "QmlTypes.h"
#include "Translations.h"

#include <QGuiApplication>
#include <QLocale>
#include <QQuickView>
#include <QScopedPointer>
#include <QTranslator>
#include <sailfishapp.h>

// Exported so the Silica booster can dlopen() the binary and call main().
Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QGuiApplication::setApplicationVersion(QStringLiteral(SALAMA_VERSION));

    QTranslator translator;
    if (Salama::loadTranslations(
            translator, QLocale(),
            SailfishApp::pathTo(QStringLiteral("translations")).toLocalFile())) {
        QGuiApplication::installTranslator(&translator);
    }

    Salama::Core core(Salama::Storage::defaultDataDirectory(),
                      Salama::Storage::defaultConfigFilePath(),
                      Salama::Storage::defaultDownloadDirectory());
    Salama::registerQmlTypes(&core);
    // Before window load: share that launched browser waits on name claim, share sheet
    // times out fast. Window signals ready later (qml/harbour-salama.qml).
    core.shareReceiver()->registerService();
    QObject::connect(app.data(), &QGuiApplication::aboutToQuit, &core, &Salama::Core::clearOnClose);
    QObject::connect(app.data(), &QGuiApplication::aboutToQuit, core.webNotifications(),
                     &Salama::WebNotifications::closeAll);

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    return QGuiApplication::exec();
}
