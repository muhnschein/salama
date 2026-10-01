// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "enterkey.h"
#include "enums.h"

#include <QImage>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQuickImageProvider>
#include <QtQml>

// Silica's plugin adds the "theme" image provider every image://theme/ source comes
// from. The stub's draws every id as the same grey square, at the size asked for when
// one is, so that an image the QML shows from the theme loads rather than failing.
class ThemeImageProvider : public QQuickImageProvider
{
public:
    ThemeImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(id)
        const bool sized = requestedSize.width() > 0 && requestedSize.height() > 0;
        QImage image(sized ? requestedSize : QSize(16, 16), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::gray);
        if (size != nullptr) {
            *size = image.size();
        }
        return image;
    }
};

// What the WebView stub's grabToImage() hands back as a grab result's image: QML has no
// way to make a QImage of its own. Size zero is a null image.
class GrabStub : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE QVariant image(int width, int height) const
    {
        if (width <= 0 || height <= 0) {
            return QVariant::fromValue(QImage());
        }
        QImage picture(width, height, QImage::Format_ARGB32_Premultiplied);
        picture.fill(Qt::darkCyan);
        return QVariant::fromValue(picture);
    }
};

class SilicaStubsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override
    {
        const QString reason = QStringLiteral("enum holder");
        qmlRegisterUncreatableType<EnterKey>(uri, 1, 0, "EnterKey",
                                             QStringLiteral("attached property"));
        qmlRegisterUncreatableType<Orientation>(uri, 1, 0, "Orientation", reason);
        qmlRegisterUncreatableType<PageStatus>(uri, 1, 0, "PageStatus", reason);
        qmlRegisterUncreatableType<Cover>(uri, 1, 0, "Cover", reason);
        qmlRegisterUncreatableType<TruncationMode>(uri, 1, 0, "TruncationMode", reason);
        qmlRegisterUncreatableType<Dock>(uri, 1, 0, "Dock", reason);
        qmlRegisterUncreatableType<OpacityRamp>(uri, 1, 0, "OpacityRamp", reason);
        qmlRegisterUncreatableType<FocusBehavior>(uri, 1, 0, "FocusBehavior", reason);
        qmlRegisterUncreatableType<PageStackAction>(uri, 1, 0, "PageStackAction", reason);
        qmlRegisterUncreatableType<TouchInteraction>(uri, 1, 0, "TouchInteraction", reason);
        qmlRegisterSingletonType<GrabStub>(
            uri, 1, 0, "GrabStub",
            [](QQmlEngine *, QJSEngine *) -> QObject * { return new GrabStub; });
    }

    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(uri)
        engine->addImageProvider(QStringLiteral("theme"), new ThemeImageProvider);
    }
};

#include "plugin.moc"
