// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#pragma once

#include <QObject>
#include <QString>

namespace Tuuli {

// Every engine-facing string lives here so QML never carries an engine quirk.
// Topics are what the platform Gecko embedding (embedlite-components) observes; the
// reference is sailfish-browser apps/core/settingmanager.cpp (Jolla Ltd., MPL-2.0).
class EngineMessages : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString clearPrivateDataTopic READ clearPrivateDataTopic CONSTANT)
    Q_PROPERTY(QString cookiesAndSiteDataPayload READ cookiesAndSiteDataPayload CONSTANT)
    Q_PROPERTY(QString cachePayload READ cachePayload CONSTANT)
    Q_PROPERTY(QString faviconScript READ faviconScript CONSTANT)
    Q_PROPERTY(QString themeColorScript READ themeColorScript CONSTANT)

public:
    explicit EngineMessages(QObject *parent = nullptr);

    QString clearPrivateDataTopic() const;
    QString cookiesAndSiteDataPayload() const;
    QString cachePayload() const;

    // Script for WebView.runJavaScript(); evaluates to the page's <link rel=icon> href
    // or an empty string. The WebView exposes no favicon property (qtmozembed
    // qmozview_defined_wrapper.h), so the page is asked directly.
    QString faviconScript() const;

    // Absolute icon URL for a page: the script result resolved against the page, or
    // the conventional /favicon.ico when the page declares none.
    Q_INVOKABLE QString resolveFavicon(const QString &pageUrl, const QString &href) const;
    Q_INVOKABLE QString defaultFavicon(const QString &pageUrl) const;

    // Script for WebView.runJavaScript(); evaluates to the page's theme-color, the
    // colour a page asks the browser to dress itself in, or an empty string. Asked of
    // the page for the same reason the favicon is: sailfish-browser has the colour as
    // a property, but on its own DeclarativeWebPage, fed by a Gecko-side message that
    // the WebView Harbour allows does not carry
    // (apps/qtmozembed/declarativewebpage.cpp).
    QString themeColorScript() const;

    // What that colour says, as something Qt can draw with, or empty when it says
    // nothing Qt understands. CSS writes colours in forms QColor does not read --
    // rgb(), and the eight-digit hex whose last pair is alpha rather than its first --
    // so those are read here. Alpha is dropped: a translucent band would show whatever
    // is behind it, which is the one thing a page's own colour must not do.
    Q_INVOKABLE static QString themeColor(const QString &value);
};

} // namespace Tuuli
