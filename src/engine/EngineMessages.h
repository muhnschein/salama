// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace Salama {

// Every engine-facing string lives here so QML never carries an engine quirk.
// Topics are what the platform Gecko embedding (embedlite-components) observes; the
// reference is sailfish-browser apps/core/settingmanager.cpp (Jolla Ltd., MPL-2.0).
class EngineMessages : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString clearPrivateDataTopic READ clearPrivateDataTopic CONSTANT)
    Q_PROPERTY(QString cookiesAndSiteDataPayload READ cookiesAndSiteDataPayload CONSTANT)
    Q_PROPERTY(QString cachePayload READ cachePayload CONSTANT)
    Q_PROPERTY(QString memoryPressureTopic READ memoryPressureTopic CONSTANT)
    Q_PROPERTY(QString heapMinimizePayload READ heapMinimizePayload CONSTANT)
    Q_PROPERTY(QString faviconScript READ faviconScript CONSTANT)
    Q_PROPERTY(QString themeColorScript READ themeColorScript CONSTANT)
    Q_PROPERTY(QString viewportScript READ viewportScript CONSTANT)
    Q_PROPERTY(QString findMessage READ findMessage CONSTANT)
    Q_PROPERTY(QString findResultMessage READ findResultMessage CONSTANT)
    Q_PROPERTY(QString searchOfferedMessage READ searchOfferedMessage CONSTANT)
    Q_PROPERTY(QString contextMenuMessage READ contextMenuMessage CONSTANT)

public:
    explicit EngineMessages(QObject *parent = nullptr);

    QString clearPrivateDataTopic() const;
    QString cookiesAndSiteDataPayload() const;
    QString cachePayload() const;

    // What sailfish-browser tells the engine after ten minutes in the background:
    // "memory-pressure" with "heap-minimize", on which Gecko frees what it can and
    // keeps what it must (apps/core/webpages.cpp). The same words from here, so the
    // engine does for this browser what it does for Jolla's.
    QString memoryPressureTopic() const;
    QString heapMinimizePayload() const;

    // Script for WebView.runJavaScript(); answers the page's <link rel=icon> href or
    // an empty string. The WebView exposes no favicon property (qtmozembed
    // qmozview_defined_wrapper.h), so the page is asked directly.
    //
    // Every script here is the **body of a function**, and has to return: the engine
    // builds one from it and calls it -- `new content.Function(script)` in
    // embedlite-components jsscripts/embedhelper.js -- so a script that merely
    // evaluates to something hands the callback undefined.
    QString faviconScript() const;

    // Absolute icon URL for a page: the script result resolved against the page, or
    // the conventional /favicon.ico when the page declares none.
    Q_INVOKABLE QString resolveFavicon(const QString &pageUrl, const QString &href) const;
    Q_INVOKABLE QString defaultFavicon(const QString &pageUrl) const;

    // Script for WebView.runJavaScript(); answers the page's theme-color, the colour
    // a page asks the browser to dress itself in, or an empty string. Asked of
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

    // Script for WebView.runJavaScript(); answers what the page's viewport meta tag
    // says, or an empty string. Asked for the reason the theme colour is: the
    // viewport-fit sailfish-browser reads comes to its own web page item, not to the
    // WebView Harbour allows (docs/DECISIONS/0013-screen-cutout.md).
    QString viewportScript() const;

    // Whether that answer asks for the whole screen, cutout and all: viewport-fit=cover
    // among its comma- or semicolon-separated settings, as CSS Round Display writes it,
    // in any case and with any spacing.
    Q_INVOKABLE static bool coversCutout(const QString &viewport);

    // Find in page, the way sailfish-browser does it: findMessage goes to the page
    // with WebView.sendAsyncMessage (apps/browser/qml/pages/components/ToolBar.qml),
    // and the page answers on findResultMessage, which is heard only once registered
    // with WebView.addMessageListener (sailfish-browser registers it in
    // apps/qtmozembed/declarativewebpage.cpp and reads it in apps/shared/WebView.qml).
    // The page's side is embedlite-components jsscripts/embedhelper.js, which keeps
    // one Finder per page.
    QString findMessage() const;
    QString findResultMessage() const;

    // A page that says it has a search of its own: <link rel="search"
    // type="application/opensearchdescription+xml">, which embedlite-components'
    // jsscripts/ContentLinkHandler.jsm tells the application of with this message, once
    // for each document and only for a link with a title and an http, https or ftp
    // address. sailfish-browser registers it with addMessageListener and adds the
    // engine to its list as it arrives (apps/qtmozembed/declarativewebpage.cpp,
    // apps/shared/WebView.qml).
    QString searchOfferedMessage() const;

    // What the message says, as {title, href, host}: the title the page gave the search,
    // the address of its OpenSearch description, and the host of the page that offered it.
    // The message is {engine: {title, href}, url: the page's address}. All three are
    // there, and empty, for a message that says nothing of the kind.
    Q_INVOKABLE static QVariantMap searchOffered(const QVariant &data);

    // A press held on the page: embedlite-components' jsscripts/ContextMenuHandler.js
    // sends this for every one, with what is under the finger -- {types, linkURL,
    // linkTitle, linkProtocol, mediaURL, contentType, ...}, types holding "link" for a
    // link and "image" for a picture. The platform's WebView listens for it already, and
    // so it is heard without asking (import/webview/WebView.qml in
    // sailfish-components-webview) (docs/DECISIONS/0046-link-menu.md).
    QString contextMenuMessage() const;

    // What the link sheet needs of that message, every key there whatever it says:
    //  * link: the address of the link pressed, or empty -- for no link, and for a
    //    javascript: one, which is a script to run rather than a place to go;
    //  * kind: "page" for a link that opens in a tab, "app" for one another
    //    application takes (mailto:, tel:, sms:, geo:, TabModel::isExternalUrl), and
    //    empty for no link;
    //  * scheme: the link's, in lower case;
    //  * title: the link's text, as one line;
    //  * image: the address of the picture pressed, an http or https one, or empty;
    //  * contentType: the picture's type, as the engine's cache has it;
    //  * address: what the sheet shows under the title -- for a link to a page its
    //    host, without www, and the path and query after it; for another application's
    //    the address without its scheme (the mailbox, the number); with no link, the
    //    picture's, the same way.
    Q_INVOKABLE static QVariantMap linkTarget(const QVariant &data);

    // What to send for a search: the text; whether to go on to the next match of it
    // rather than start over; and which way. An empty text is the message that ends
    // the search: on it embedhelper.js takes the highlight away and destroys its
    // Finder. Without it the last match stays highlighted.
    Q_INVOKABLE QVariantMap findRequest(const QString &text, bool again, bool backwards) const;

    // Whether the page's answer says the text is there. The answer is {"r": result},
    // result being one of nsITypeAheadFind's FIND_FOUND 0, FIND_NOTFOUND 1,
    // FIND_WRAPPED 2 -- found, after going round the end of the page -- and
    // FIND_PENDING 3 (gecko-dev toolkit/components/typeaheadfind/nsITypeAheadFind.idl).
    // An answer that carries no number says nothing was found.
    Q_INVOKABLE static bool findFound(const QVariant &data);

    // What the engine is told for a level of PrivacySettings::TrackingProtection: a list of
    // {name, value}, each for WebEngineSettings.setPreference(). Every level names the
    // same preferences in the same order, so a move between levels leaves nothing of
    // the last one in the profile. Standard and Strict are Firefox's own categories
    // (browser/components/protections/ContentBlockingPrefs.sys.mjs), Off is the
    // engine's defaults, and a level out of range is Standard. Trackers are blocked
    // through the content classifier, the one list-driven path this embedding keeps
    // fed (docs/DECISIONS/0023-tracking-protection.md).
    //
    // Cookies are the one preference with two owners: at Standard and Strict it is the
    // level's, and at Off it is the reader's own choice, a SitePermissionSettings::Cookies
    // value, so that turning tracking protection off lets the choice of what cookies to
    // accept take its place (docs/DECISIONS/0039-site-permissions.md). A choice out of
    // range is Block cross-site.
    Q_INVOKABLE static QVariantList trackingProtectionPreferences(int level, int cookies);

    // What the engine is told for the defaults of Site permissions, in the same
    // {name, value} form: whether a page may open a window of its own accord, and
    // whether a site that has not been given or refused a location, the camera or the
    // microphone is asked or refused outright (docs/DECISIONS/0039-site-permissions.md).
    // The notifications' default is NotificationPermissions::defaultPreference().
    Q_INVOKABLE static QVariantList sitePermissionPreferences(bool popupsAllowed,
                                                              bool locationBlocked,
                                                              bool cameraBlocked,
                                                              bool microphoneBlocked);

    // What the engine is told for a Settings::WebsiteColors choice, in the same
    // {name, value} form: whether the system it draws for uses a dark theme, which is
    // what a page's prefers-color-scheme reads. Gecko takes the answer from its widget
    // toolkit unless the preference ui.systemUsesDarkTheme says otherwise
    // (widget/nsXPLookAndFeel.cpp), and that is the one way to it Harbour leaves: 1 is
    // dark and 0 light. Automatic is dark on a dark ambience; a choice out of range is
    // Automatic (docs/DECISIONS/0035-website-colours.md).
    Q_INVOKABLE static QVariantList websiteColorPreferences(int colors, bool darkAmbience);

    // What the engine is told for the Privacy switches Do not track and Enable
    // JavaScript, in the same {name, value} form: the preferences sailfish-browser's
    // WebEngineSettings.doNotTrack and javascriptEnabled write
    // (qtmozembed qmozenginesettings.cpp), privacy.donottrackheader.enabled and
    // javascript.enabled.
    Q_INVOKABLE static QVariantList contentPreferences(bool doNotTrack, bool javascript);
};

} // namespace Salama
