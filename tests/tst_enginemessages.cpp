// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/EngineMessages.h"
#include "settings/PrivacySettings.h"
#include "settings/Settings.h"
#include "settings/SitePermissionSettings.h"

#include <QtTest>

using Salama::EngineMessages;
using Salama::PrivacySettings;
using Salama::Settings;
using Salama::SitePermissionSettings;

namespace {

// The cookies the reader chose, Block cross-site unless a test says otherwise: only what
// Off does with it is a test of its own.
QVariantMap trackingValues(int level, int cookies = SitePermissionSettings::CookiesBlockCrossSite)
{
    QVariantMap values;
    for (const QVariant &entry : EngineMessages::trackingProtectionPreferences(level, cookies)) {
        const QVariantMap preference = entry.toMap();
        values.insert(preference.value(QStringLiteral("name")).toString(),
                      preference.value(QStringLiteral("value")));
    }
    return values;
}

QStringList trackingNames(int level)
{
    QStringList names;
    for (const QVariant &entry : EngineMessages::trackingProtectionPreferences(
             level, SitePermissionSettings::CookiesBlockCrossSite)) {
        names.append(entry.toMap().value(QStringLiteral("name")).toString());
    }
    return names;
}

// Split by hand for an empty list: Qt 5.6 has no Qt::SkipEmptyParts, and host Qt
// deprecates QString::SkipEmptyParts.
QStringList features(const QVariant &engines)
{
    const QString list = engines.toString();
    return list.isEmpty() ? QStringList() : list.split(QLatin1Char(','));
}

const char *const ContentBlocking = "privacy.trackingprotection.content.protection.engines";
const char *const ContentAnnotation = "privacy.trackingprotection.content.annotation.engines";

} // namespace

class tst_enginemessages : public QObject
{
    Q_OBJECT

private slots:
    void constants();
    void defaultFavicon_data();
    void defaultFavicon();
    void resolveFavicon_data();
    void resolveFavicon();
    void themeColor_data();
    void themeColor();
    void findRequest();
    void searchOffered();
    void linkTarget_data();
    void linkTarget();
    void linkTargetOfNothing();
    void findFound_data();
    void findFound();
    void trackingProtectionNamesTheSamePreferences();
    void trackingProtectionLevels();
    void trackingProtectionFeatures();
    void cookiesAtOffAreTheReadersChoice();
    void sitePermissionDefaults();
    void websiteColors_data();
    void websiteColors();
    void coversCutout_data();
    void coversCutout();
    void contentPreferences();
};

void tst_enginemessages::constants()
{
    EngineMessages messages;
    QCOMPARE(messages.clearPrivateDataTopic(), QStringLiteral("clear-private-data"));
    QCOMPARE(messages.cookiesAndSiteDataPayload(), QStringLiteral("cookies-and-site-data"));
    QCOMPARE(messages.cachePayload(), QStringLiteral("cache"));
    // The engine builds a function from the script and calls it -- embedhelper.js
    // does `new content.Function(script)` -- so a script is a function body and has
    // to return. One that only evaluates to something hands the callback undefined,
    // which is what the favicon script did: every page fell back to /favicon.ico.
    QVERIFY(messages.faviconScript().contains(QStringLiteral("icon")));
    QVERIFY(messages.faviconScript().contains(QStringLiteral("return ")));
    QVERIFY(!messages.faviconScript().contains(QStringLiteral("function")));
    QVERIFY(messages.themeColorScript().contains(QStringLiteral("theme-color")));
    QVERIFY(messages.themeColorScript().contains(QStringLiteral("return ")));
    QVERIFY(!messages.themeColorScript().contains(QStringLiteral("function")));
    QVERIFY(messages.viewportScript().contains(QStringLiteral("viewport")));
    QVERIFY(messages.viewportScript().contains(QStringLiteral("return ")));
    QVERIFY(!messages.viewportScript().contains(QStringLiteral("function")));
    // Find in page: what embedhelper.js listens for, and what it answers on.
    QCOMPARE(messages.findMessage(), QStringLiteral("embedui:find"));
    QCOMPARE(messages.findResultMessage(), QStringLiteral("embed:find"));
}

// What ContentLinkHandler.jsm sends for a page that has a search of its own, and what is
// made of it: the title and the address of the description, and the page's host.
void tst_enginemessages::searchOffered()
{
    EngineMessages messages;
    QCOMPARE(messages.searchOfferedMessage(), QStringLiteral("Link:AddSearch"));

    const QVariantMap engine{
        {QStringLiteral("title"), QStringLiteral("Find")},
        {QStringLiteral("href"), QStringLiteral("https://cdn.example/os.xml")}};
    const QVariantMap offered = EngineMessages::searchOffered(
        QVariantMap{{QStringLiteral("engine"), engine},
                    {QStringLiteral("url"), QStringLiteral("https://www.find.example/page?x=1")}});
    QCOMPARE(offered.value(QStringLiteral("title")).toString(), QStringLiteral("Find"));
    QCOMPARE(offered.value(QStringLiteral("href")).toString(),
             QStringLiteral("https://cdn.example/os.xml"));
    // The page's, not the description's, and without "www.".
    QCOMPARE(offered.value(QStringLiteral("host")).toString(), QStringLiteral("find.example"));
    QCOMPARE(offered.count(), 3);

    // A page with no address to speak of is offered by the host the description is on.
    const QVariantMap unplaced = EngineMessages::searchOffered(
        QVariantMap{{QStringLiteral("engine"), engine},
                    {QStringLiteral("url"), QStringLiteral("about:blank")}});
    QCOMPARE(unplaced.value(QStringLiteral("host")).toString(), QStringLiteral("cdn.example"));
    QCOMPARE(EngineMessages::searchOffered(QVariantMap{{QStringLiteral("engine"), engine}})
                 .value(QStringLiteral("host"))
                 .toString(),
             QStringLiteral("cdn.example"));

    // Anything else says nothing, and has all three in it all the same.
    for (const QVariant &data : {QVariant(), QVariant(QStringLiteral("text")), QVariant(42),
                                 QVariant(QVariantMap{{QStringLiteral("engine"), 7}})}) {
        const QVariantMap nothing = EngineMessages::searchOffered(data);
        QCOMPARE(nothing.count(), 3);
        QVERIFY(nothing.value(QStringLiteral("title")).toString().isEmpty());
        QVERIFY(nothing.value(QStringLiteral("href")).toString().isEmpty());
    }
}

// What ContextMenuHandler.js says of a press held on the page, as the link sheet reads it.
void tst_enginemessages::linkTarget_data()
{
    QTest::addColumn<QVariantMap>("message");
    QTest::addColumn<QString>("link");
    QTest::addColumn<QString>("kind");
    QTest::addColumn<QString>("image");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("address");

    const auto link = [](const QString &url, const QString &title) {
        return QVariantMap{{QStringLiteral("types"), QStringList{QStringLiteral("link")}},
                           {QStringLiteral("linkURL"), url},
                           {QStringLiteral("linkTitle"), title}};
    };
    // The link's text as one line: a link's textContent keeps the page's line breaks.
    QTest::newRow("page") << link(QStringLiteral("https://www.trails.example/walks/ridge-loop"),
                                  QStringLiteral("  The ridge\n   loop "))
                          << QStringLiteral("https://www.trails.example/walks/ridge-loop")
                          << QStringLiteral("page") << QString() << QStringLiteral("The ridge loop")
                          << QStringLiteral("trails.example/walks/ridge-loop");
    QTest::newRow("bare host") << link(QStringLiteral("https://trails.example/"), QString())
                               << QStringLiteral("https://trails.example/")
                               << QStringLiteral("page") << QString() << QString()
                               << QStringLiteral("trails.example");
    QTest::newRow("query, decoded")
        << link(QStringLiteral("https://trails.example/s%C3%B6k?walk=1"), QStringLiteral("Search"))
        << QStringLiteral("https://trails.example/s%C3%B6k?walk=1") << QStringLiteral("page")
        << QString() << QStringLiteral("Search")
        << QStringLiteral("trails.example/s\u00f6k?walk=1");
    // What another application takes is shown without its scheme.
    QTest::newRow("mailto") << link(QStringLiteral("mailto:walks@trails.example"),
                                    QStringLiteral("Write to us"))
                            << QStringLiteral("mailto:walks@trails.example")
                            << QStringLiteral("app") << QString() << QStringLiteral("Write to us")
                            << QStringLiteral("walks@trails.example");
    QTest::newRow("tel") << link(QStringLiteral("tel:+358401234567"), QString())
                         << QStringLiteral("tel:+358401234567") << QStringLiteral("app")
                         << QString() << QString() << QStringLiteral("+358401234567");
    QTest::newRow("geo") << link(QStringLiteral("GEO:60.17,24.94"), QString())
                         << QStringLiteral("GEO:60.17,24.94") << QStringLiteral("app") << QString()
                         << QString() << QStringLiteral("60.17,24.94");
    // A script to run is no place to go.
    QTest::newRow("javascript") << link(QStringLiteral("javascript:void(0)"),
                                        QStringLiteral("More"))
                                << QString() << QString() << QString() << QStringLiteral("More")
                                << QString();
    QTest::newRow("no address") << link(QString(), QStringLiteral("More")) << QString() << QString()
                                << QString() << QStringLiteral("More") << QString();

    // A picture that is a link: both, and the link's address under the title.
    QTest::newRow("linked image")
        << QVariantMap{{QStringLiteral("types"),
                        QStringList{QStringLiteral("image"), QStringLiteral("link")}},
                       {QStringLiteral("linkURL"),
                        QStringLiteral("https://trails.example/maps/ridge")},
                       {QStringLiteral("mediaURL"),
                        QStringLiteral("https://cdn.example/ridge.jpg")},
                       {QStringLiteral("contentType"), QStringLiteral("image/jpeg")}}
        << QStringLiteral("https://trails.example/maps/ridge") << QStringLiteral("page")
        << QStringLiteral("https://cdn.example/ridge.jpg") << QString()
        << QStringLiteral("trails.example/maps/ridge");
    // A picture alone: the picture's address.
    QTest::newRow("image") << QVariantMap{{QStringLiteral("types"),
                                           QStringList{QStringLiteral("image")}},
                                          {QStringLiteral("mediaURL"),
                                           QStringLiteral("https://cdn.example/photos/ridge.jpg")},
                                          {QStringLiteral("linkTitle"), QStringLiteral("Dawn")}}
                           << QString() << QString()
                           << QStringLiteral("https://cdn.example/photos/ridge.jpg")
                           << QStringLiteral("Dawn")
                           << QStringLiteral("cdn.example/photos/ridge.jpg");
    // Only what the web can give again: not one drawn from the page's own data.
    QTest::newRow("data image")
        << QVariantMap{{QStringLiteral("types"), QStringList{QStringLiteral("image")}},
                       {QStringLiteral("mediaURL"), QStringLiteral("data:image/png;base64,AAAA")}}
        << QString() << QString() << QString() << QString() << QString();
    // An address the message carries for something it does not say is there is not used.
    QTest::newRow("text")
        << QVariantMap{{QStringLiteral("types"), QStringList{QStringLiteral("content-text")}},
                       {QStringLiteral("linkURL"), QStringLiteral("https://a.example/")},
                       {QStringLiteral("mediaURL"), QStringLiteral("https://a.example/b.png")}}
        << QString() << QString() << QString() << QString() << QString();
}

void tst_enginemessages::linkTarget()
{
    QFETCH(QVariantMap, message);
    QFETCH(QString, link);
    QFETCH(QString, kind);
    QFETCH(QString, image);
    QFETCH(QString, title);
    QFETCH(QString, address);

    EngineMessages messages;
    QCOMPARE(messages.contextMenuMessage(), QStringLiteral("Content:ContextMenu"));
    const QVariantMap target = EngineMessages::linkTarget(message);
    QCOMPARE(target.value(QStringLiteral("link")).toString(), link);
    QCOMPARE(target.value(QStringLiteral("kind")).toString(), kind);
    QCOMPARE(target.value(QStringLiteral("scheme")).toString(),
             link.isEmpty() ? QString() : link.section(QLatin1Char(':'), 0, 0).toLower());
    QCOMPARE(target.value(QStringLiteral("image")).toString(), image);
    QCOMPARE(target.value(QStringLiteral("title")).toString(), title);
    QCOMPARE(target.value(QStringLiteral("address")).toString(), address);
    QCOMPARE(target.value(QStringLiteral("contentType")).toString(),
             message.value(QStringLiteral("contentType")).toString());
    QCOMPARE(target.count(), 7);
}

// Whatever arrives, every key is there for QML to read, and empty.
void tst_enginemessages::linkTargetOfNothing()
{
    for (const QVariant &data : {QVariant(), QVariant(QStringLiteral("text")), QVariant(42),
                                 QVariant(QVariantMap{{QStringLiteral("types"), 7}})}) {
        const QVariantMap nothing = EngineMessages::linkTarget(data);
        QCOMPARE(nothing.count(), 7);
        for (auto it = nothing.cbegin(); it != nothing.cend(); ++it) {
            QVERIFY2(it.value().toString().isEmpty(), qPrintable(it.key()));
        }
    }
}

// The three fields embedhelper.js reads off the message, by the names it reads them by.
void tst_enginemessages::findRequest()
{
    EngineMessages messages;
    const QVariantMap first = messages.findRequest(QStringLiteral("salama"), false, false);
    QCOMPARE(first.value(QStringLiteral("text")).toString(), QStringLiteral("salama"));
    QCOMPARE(first.value(QStringLiteral("again")), QVariant(false));
    QCOMPARE(first.value(QStringLiteral("backwards")), QVariant(false));
    QCOMPARE(first.count(), 3);

    const QVariantMap previous = messages.findRequest(QStringLiteral("salama"), true, true);
    QCOMPARE(previous.value(QStringLiteral("again")), QVariant(true));
    QCOMPARE(previous.value(QStringLiteral("backwards")), QVariant(true));

    // The message that ends the search is the same one with no text in it.
    const QVariantMap end = messages.findRequest(QString(), false, false);
    QVERIFY(end.contains(QStringLiteral("text")));
    QVERIFY(end.value(QStringLiteral("text")).toString().isEmpty());
}

// The page answers {"r": result} with nsITypeAheadFind's result. Found, and found after
// going round the end, are the two that mean the text is there; anything the page did
// not say as a number means it is not.
void tst_enginemessages::findFound_data()
{
    QTest::addColumn<QVariant>("data");
    QTest::addColumn<bool>("expected");
    const QString r = QStringLiteral("r");
    // JSON has one kind of number, and the device's Qt 5.6 reads every one as a
    // double, so that is how the engine's answer arrives.
    QTest::newRow("found") << QVariant(QVariantMap{{r, 0.0}}) << true;
    QTest::newRow("not found") << QVariant(QVariantMap{{r, 1.0}}) << false;
    QTest::newRow("wrapped") << QVariant(QVariantMap{{r, 2.0}}) << true;
    QTest::newRow("pending") << QVariant(QVariantMap{{r, 3.0}}) << false;
    // QML hands an integral number over as an int.
    QTest::newRow("found int") << QVariant(QVariantMap{{r, 0}}) << true;
    QTest::newRow("wrapped int") << QVariant(QVariantMap{{r, 2}}) << true;
    QTest::newRow("not found int") << QVariant(QVariantMap{{r, 1}}) << false;
    // Qt reads a whole number in JSON as a qlonglong from 5.15 on.
    QTest::newRow("found longlong") << QVariant(QVariantMap{{r, 0LL}}) << true;
    QTest::newRow("wrapped longlong") << QVariant(QVariantMap{{r, 2LL}}) << true;
    QTest::newRow("pending longlong") << QVariant(QVariantMap{{r, 3LL}}) << false;
    // Nothing is known to hand over an unsigned number, but it is a number all the same.
    QTest::newRow("wrapped uint") << QVariant(QVariantMap{{r, 2U}}) << true;
    QTest::newRow("not found uint") << QVariant(QVariantMap{{r, 1U}}) << false;
    QTest::newRow("found ulonglong") << QVariant(QVariantMap{{r, 0ULL}}) << true;
    QTest::newRow("not found ulonglong") << QVariant(QVariantMap{{r, 1ULL}}) << false;
    QTest::newRow("unknown") << QVariant(QVariantMap{{r, 7.0}}) << false;
    QTest::newRow("fraction") << QVariant(QVariantMap{{r, 0.5}}) << false;
    QTest::newRow("missing") << QVariant(QVariantMap{}) << false;
    QTest::newRow("null") << QVariant(QVariantMap{{r, QVariant()}}) << false;
    // QVariant reads 0 out of these, which would be FIND_FOUND.
    QTest::newRow("false") << QVariant(QVariantMap{{r, false}}) << false;
    QTest::newRow("string") << QVariant(QVariantMap{{r, QStringLiteral("0")}}) << false;
    QTest::newRow("empty string") << QVariant(QVariantMap{{r, QString()}}) << false;
    QTest::newRow("no map") << QVariant(0.0) << false;
    QTest::newRow("nothing") << QVariant() << false;
    QTest::newRow("text") << QVariant(QStringLiteral("{\"r\": 0}")) << false;
}

void tst_enginemessages::findFound()
{
    QFETCH(QVariant, data);
    QFETCH(bool, expected);
    QCOMPARE(EngineMessages::findFound(data), expected);
}

// What a page's theme-color says, read into something Qt can draw with. CSS writes
// colours in forms QColor does not, and a page can write anything at all.
void tst_enginemessages::themeColor_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<QString>("expected");
    const QString blue = QStringLiteral("#123456");
    QTest::newRow("hex") << blue << blue;
    QTest::newRow("padded") << QStringLiteral("  #123456  ") << blue;
    QTest::newRow("short hex") << QStringLiteral("#abc") << QStringLiteral("#aabbcc");
    // CSS puts alpha last in an eight-digit hex; QColor reads the same string with
    // alpha first, so #123456ff would come out as #3456ff without this.
    QTest::newRow("hex with alpha") << QStringLiteral("#123456ff") << blue;
    QTest::newRow("named") << QStringLiteral("darkslateblue") << QStringLiteral("#483d8b");
    QTest::newRow("rgb") << QStringLiteral("rgb(18, 52, 86)") << blue;
    QTest::newRow("rgb spaces") << QStringLiteral("rgb(18 52 86)") << blue;
    // Alpha is dropped rather than honoured: a band that showed what is behind it
    // would not be the page's colour any more.
    QTest::newRow("rgba") << QStringLiteral("rgba(18, 52, 86, 0.5)") << blue;
    QTest::newRow("rgb clamped") << QStringLiteral("rgb(300, 52, 86)") << QStringLiteral("#ff3456");
    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("nonsense") << QStringLiteral("very blue") << QString();
    QTest::newRow("script leftovers") << QStringLiteral("undefined") << QString();
    QTest::newRow("rgb too few") << QStringLiteral("rgb(18, 52)") << QString();
}

void tst_enginemessages::themeColor()
{
    QFETCH(QString, value);
    QFETCH(QString, expected);
    QCOMPARE(EngineMessages::themeColor(value), expected);
}

void tst_enginemessages::defaultFavicon_data()
{
    QTest::addColumn<QString>("page");
    QTest::addColumn<QString>("expected");
    QTest::newRow("https") << QStringLiteral("https://example.org/deep/path?x=1")
                           << QStringLiteral("https://example.org/favicon.ico");
    QTest::newRow("http port") << QStringLiteral("http://example.org:8080/")
                               << QStringLiteral("http://example.org:8080/favicon.ico");
    QTest::newRow("about") << QStringLiteral("about:blank") << QString();
    QTest::newRow("file") << QStringLiteral("file:///tmp/x.html") << QString();
    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("no host") << QStringLiteral("https:///nohost") << QString();
}

void tst_enginemessages::defaultFavicon()
{
    QFETCH(QString, page);
    QFETCH(QString, expected);
    EngineMessages messages;
    QCOMPARE(messages.defaultFavicon(page), expected);
}

void tst_enginemessages::resolveFavicon_data()
{
    QTest::addColumn<QString>("page");
    QTest::addColumn<QString>("href");
    QTest::addColumn<QString>("expected");
    const QString page = QStringLiteral("https://example.org/news/today");
    QTest::newRow("relative") << page << QStringLiteral("icons/site.png")
                              << QStringLiteral("https://example.org/news/icons/site.png");
    QTest::newRow("root relative")
        << page << QStringLiteral("/i.png") << QStringLiteral("https://example.org/i.png");
    QTest::newRow("absolute") << page << QStringLiteral("https://cdn.example.net/i.ico")
                              << QStringLiteral("https://cdn.example.net/i.ico");
    QTest::newRow("data") << page << QStringLiteral("data:image/png;base64,AAAA")
                          << QStringLiteral("data:image/png;base64,AAAA");
    QTest::newRow("empty") << page << QString()
                           << QStringLiteral("https://example.org/favicon.ico");
    QTest::newRow("spaces") << page << QStringLiteral("   ")
                            << QStringLiteral("https://example.org/favicon.ico");
    QTest::newRow("javascript") << page << QStringLiteral("javascript:alert(1)")
                                << QStringLiteral("https://example.org/favicon.ico");
    QTest::newRow("no page") << QString() << QStringLiteral("/i.png") << QString();
}

void tst_enginemessages::resolveFavicon()
{
    QFETCH(QString, page);
    QFETCH(QString, href);
    QFETCH(QString, expected);
    EngineMessages messages;
    QCOMPARE(messages.resolveFavicon(page, href), expected);
}

// Every level writes every preference, in one order and with one type each, so that
// moving from Strict to Off leaves nothing of Strict in the profile, and a number is
// never handed to the engine as text: setPreference() picks the engine's setter by the
// value's type, and the engine refuses a value of the wrong one.
void tst_enginemessages::trackingProtectionNamesTheSamePreferences()
{
    QStringList names = trackingNames(PrivacySettings::TrackingProtectionStandard);
    QCOMPARE(names.count(), 12);
    QCOMPARE(names.removeDuplicates(), 0);
    QCOMPARE(trackingNames(PrivacySettings::TrackingProtectionOff), names);
    QCOMPARE(trackingNames(PrivacySettings::TrackingProtectionStrict), names);

    for (const QVariant &entry : EngineMessages::trackingProtectionPreferences(
             PrivacySettings::TrackingProtectionStandard,
             SitePermissionSettings::CookiesBlockCrossSite)) {
        QCOMPARE(entry.toMap().count(), 2);
    }
    const QVariantMap off = trackingValues(PrivacySettings::TrackingProtectionOff);
    const QVariantMap standard = trackingValues(PrivacySettings::TrackingProtectionStandard);
    const QVariantMap strict = trackingValues(PrivacySettings::TrackingProtectionStrict);
    for (const QString &name : names) {
        QCOMPARE(off.value(name).userType(), standard.value(name).userType());
        QCOMPARE(strict.value(name).userType(), standard.value(name).userType());
    }
    QCOMPARE(standard.value(QStringLiteral("network.cookie.cookieBehavior")).userType(),
             int(QMetaType::Int));
    QCOMPARE(standard.value(QStringLiteral("privacy.bounceTrackingProtection.mode")).userType(),
             int(QMetaType::Int));
    QCOMPARE(standard.value(QLatin1String(ContentBlocking)).userType(), int(QMetaType::QString));
    QCOMPARE(standard.value(QStringLiteral("privacy.fingerprintingProtection")).userType(),
             int(QMetaType::Bool));
}

void tst_enginemessages::trackingProtectionLevels()
{
    const QString cookies = QStringLiteral("network.cookie.cookieBehavior");
    const QString blocking =
        QStringLiteral("privacy.trackingprotection.content.protection.enabled");
    const QString annotation =
        QStringLiteral("privacy.trackingprotection.content.annotation.enabled");
    const QString bounceTracking = QStringLiteral("privacy.bounceTrackingProtection.mode");
    const QStringList strictOnly{
        QStringLiteral("privacy.annotate_channels.strict_list.enabled"),
        QStringLiteral("privacy.fingerprintingProtection"),
        QStringLiteral("privacy.query_stripping.enabled"),
        QStringLiteral("network.http.referer.disallowCrossSiteRelaxingDefault.top_navigation"),
    };
    const QString convenience =
        QStringLiteral("privacy.trackingprotection.allow_list.convenience.enabled");

    // Off is the engine as it comes -- nothing classified, bounce tracking watched and
    // never acted on -- save the cookies, which are the reader's choice: Block cross-site
    // here (cookiesAtOffAreTheReadersChoice() has the others).
    const QVariantMap off = trackingValues(PrivacySettings::TrackingProtectionOff);
    QCOMPARE(off.value(cookies), QVariant(1));
    QCOMPARE(off.value(blocking), QVariant(false));
    QCOMPARE(off.value(annotation), QVariant(false));
    QCOMPARE(off.value(bounceTracking), QVariant(3));
    QVERIFY(features(off.value(QLatin1String(ContentBlocking))).isEmpty());
    QVERIFY(features(off.value(QLatin1String(ContentAnnotation))).isEmpty());
    for (const QString &name : strictOnly) {
        QCOMPARE(off.value(name), QVariant(false));
    }
    QCOMPARE(off.value(convenience), QVariant(true));

    // Standard is Firefox's: Total Cookie Protection, and fingerprinters and
    // cryptominers blocked.
    const QVariantMap standard = trackingValues(PrivacySettings::TrackingProtectionStandard);
    QCOMPARE(standard.value(cookies), QVariant(5));
    QCOMPARE(standard.value(blocking), QVariant(true));
    QCOMPARE(standard.value(annotation), QVariant(true));
    QCOMPARE(standard.value(bounceTracking), QVariant(3));
    QCOMPARE(features(standard.value(QLatin1String(ContentBlocking))),
             (QStringList{QStringLiteral("fingerprinters"), QStringLiteral("cryptominers"),
                          QStringLiteral("major-exceptions"), QStringLiteral("minor-exceptions")}));
    for (const QString &name : strictOnly) {
        QCOMPARE(standard.value(name), QVariant(false));
    }
    QCOMPARE(standard.value(convenience), QVariant(true));

    // Strict blocks every tracker list, and switches on what Standard leaves off.
    const QVariantMap strict = trackingValues(PrivacySettings::TrackingProtectionStrict);
    QCOMPARE(strict.value(cookies), QVariant(5));
    QCOMPARE(strict.value(bounceTracking), QVariant(1));
    for (const QString &name : strictOnly) {
        QCOMPARE(strict.value(name), QVariant(true));
    }
    QCOMPARE(strict.value(convenience), QVariant(false));
    const QStringList strictBlocking = features(strict.value(QLatin1String(ContentBlocking)));
    for (const QString &feature : features(standard.value(QLatin1String(ContentBlocking)))) {
        if (feature != QLatin1String("minor-exceptions")) {
            QVERIFY2(strictBlocking.contains(feature), qPrintable(feature));
        }
    }
    QVERIFY(strictBlocking.contains(QStringLiteral("trackers")));
    QVERIFY(strictBlocking.contains(QStringLiteral("social-trackers")));
    QVERIFY(!strictBlocking.contains(QStringLiteral("minor-exceptions")));

    // A level from outside the range is Standard, as Settings reads one back.
    QCOMPARE(trackingValues(3), standard);
    QCOMPARE(trackingValues(-1), standard);
}

// The feature names are the engine's own, and the order it needs them in: a blocking
// list ends with its exception-only features, and annotation has none of them.
void tst_enginemessages::trackingProtectionFeatures()
{
    // kFeatures in gecko-dev toolkit/components/content-classifier/
    // ContentClassifierService.cpp, less the two test-only ones.
    const QStringList engine{
        QStringLiteral("trackers"),        QStringLiteral("trackers-content"),
        QStringLiteral("social-trackers"), QStringLiteral("fingerprinters"),
        QStringLiteral("email-trackers"),  QStringLiteral("cryptominers"),
    };
    const QStringList exceptions{QStringLiteral("minor-exceptions"),
                                 QStringLiteral("major-exceptions")};

    for (int level : {int(PrivacySettings::TrackingProtectionStandard),
                      int(PrivacySettings::TrackingProtectionStrict)}) {
        const QVariantMap values = trackingValues(level);
        const QStringList blocking = features(values.value(QLatin1String(ContentBlocking)));
        bool inExceptions = false;
        for (const QString &feature : blocking) {
            QVERIFY2(engine.contains(feature) || exceptions.contains(feature), qPrintable(feature));
            if (exceptions.contains(feature)) {
                inExceptions = true;
            } else {
                QVERIFY2(!inExceptions, qPrintable(feature));
            }
        }
        QVERIFY(inExceptions);
        // The level-2 list is only ever annotated.
        QVERIFY(!blocking.contains(QStringLiteral("trackers-content")));

        const QStringList annotation = features(values.value(QLatin1String(ContentAnnotation)));
        QVERIFY(!annotation.isEmpty());
        for (const QString &feature : annotation) {
            QVERIFY2(engine.contains(feature), qPrintable(feature));
        }
        QCOMPARE(annotation.contains(QStringLiteral("trackers-content")),
                 level == PrivacySettings::TrackingProtectionStrict);
    }
}

// The cookies the reader chose count only while tracking protection is off: Standard
// and Strict are Firefox's, whatever was chosen, and the choice is the one preference
// of the twelve that differs.
void tst_enginemessages::cookiesAtOffAreTheReadersChoice()
{
    const QString cookies = QStringLiteral("network.cookie.cookieBehavior");
    const int off = PrivacySettings::TrackingProtectionOff;
    QCOMPARE(trackingValues(off, SitePermissionSettings::CookiesAllowAll).value(cookies),
             QVariant(0));
    QCOMPARE(trackingValues(off, SitePermissionSettings::CookiesBlockCrossSite).value(cookies),
             QVariant(1));
    QCOMPARE(trackingValues(off, SitePermissionSettings::CookiesBlockAll).value(cookies),
             QVariant(2));
    // A choice out of range is the default, and the number reaches the engine as an
    // integer.
    QCOMPARE(trackingValues(off, 7).value(cookies), QVariant(1));
    QCOMPARE(trackingValues(off, -1).value(cookies), QVariant(1));
    QCOMPARE(trackingValues(off, 2).value(cookies).userType(), int(QMetaType::Int));

    for (const int level : {int(PrivacySettings::TrackingProtectionStandard),
                            int(PrivacySettings::TrackingProtectionStrict)}) {
        for (const int choice : {0, 1, 2}) {
            QCOMPARE(trackingValues(level, choice).value(cookies), QVariant(5));
        }
    }
    // Nothing but the cookies follows the choice.
    QVariantMap allowAll = trackingValues(off, SitePermissionSettings::CookiesAllowAll);
    QVariantMap blockAll = trackingValues(off, SitePermissionSettings::CookiesBlockAll);
    allowAll.remove(cookies);
    blockAll.remove(cookies);
    QCOMPARE(allowAll, blockAll);
}

// The defaults of Site permissions: pop-ups are blocked by a boolean preference that
// says so, the rest are asked about (0) or refused outright (2), and a location is told
// under both names the engine is known by.
void tst_enginemessages::sitePermissionDefaults()
{
    const auto values = [](bool popups, bool location, bool camera, bool microphone) {
        QVariantMap map;
        for (const QVariant &entry :
             EngineMessages::sitePermissionPreferences(popups, location, camera, microphone)) {
            map.insert(entry.toMap().value(QStringLiteral("name")).toString(),
                       entry.toMap().value(QStringLiteral("value")));
        }
        return map;
    };
    const QString popups = QStringLiteral("dom.disable_open_during_load");

    // As it comes: pop-ups blocked, the rest asked.
    const QVariantMap asked = values(false, false, false, false);
    QCOMPARE(asked.count(), 5);
    QCOMPARE(asked.value(popups), QVariant(true));
    QCOMPARE(asked.value(popups).userType(), int(QMetaType::Bool));
    for (const QString &name : {QStringLiteral("permissions.default.geo"),
                                QStringLiteral("permissions.default.geolocation"),
                                QStringLiteral("permissions.default.camera"),
                                QStringLiteral("permissions.default.microphone")}) {
        QCOMPARE(asked.value(name), QVariant(0));
        QCOMPARE(asked.value(name).userType(), int(QMetaType::Int));
    }

    // Each choice moves its own preferences and no other.
    QVariantMap changed = values(true, false, false, false);
    QCOMPARE(changed.value(popups), QVariant(false));
    changed.insert(popups, true);
    QCOMPARE(changed, asked);

    changed = values(false, true, false, false);
    QCOMPARE(changed.value(QStringLiteral("permissions.default.geo")), QVariant(2));
    QCOMPARE(changed.value(QStringLiteral("permissions.default.geolocation")), QVariant(2));
    QCOMPARE(changed.value(QStringLiteral("permissions.default.camera")), QVariant(0));
    QCOMPARE(changed.value(QStringLiteral("permissions.default.microphone")), QVariant(0));

    changed = values(false, false, true, false);
    QCOMPARE(changed.value(QStringLiteral("permissions.default.camera")), QVariant(2));
    QCOMPARE(changed.value(QStringLiteral("permissions.default.microphone")), QVariant(0));
    QCOMPARE(changed.value(QStringLiteral("permissions.default.geo")), QVariant(0));

    changed = values(false, false, false, true);
    QCOMPARE(changed.value(QStringLiteral("permissions.default.microphone")), QVariant(2));
    QCOMPARE(changed.value(QStringLiteral("permissions.default.camera")), QVariant(0));
}

void tst_enginemessages::websiteColors_data()
{
    QTest::addColumn<int>("colors");
    QTest::addColumn<bool>("darkAmbience");
    QTest::addColumn<int>("dark");

    QTest::newRow("automatic, dark ambience") << int(Settings::WebsiteColorsAutomatic) << true << 1;
    QTest::newRow("automatic, light ambience")
        << int(Settings::WebsiteColorsAutomatic) << false << 0;
    QTest::newRow("light, dark ambience") << int(Settings::WebsiteColorsLight) << true << 0;
    QTest::newRow("dark, light ambience") << int(Settings::WebsiteColorsDark) << false << 1;
    QTest::newRow("out of range is automatic") << 9 << true << 1;
}

// One preference, the engine's own override of the toolkit's dark theme, and an int:
// Gecko reads it as one (docs/DECISIONS/0035-website-colours.md).
void tst_enginemessages::websiteColors()
{
    QFETCH(int, colors);
    QFETCH(bool, darkAmbience);
    QFETCH(int, dark);

    const QVariantList preferences = EngineMessages::websiteColorPreferences(colors, darkAmbience);
    QCOMPARE(preferences.count(), 1);
    const QVariantMap preference = preferences.first().toMap();
    QCOMPARE(preference.value(QStringLiteral("name")).toString(),
             QStringLiteral("ui.systemUsesDarkTheme"));
    QCOMPARE(preference.value(QStringLiteral("value")).type(), QVariant::Int);
    QCOMPARE(preference.value(QStringLiteral("value")).toInt(), dark);
}

void tst_enginemessages::coversCutout_data()
{
    QTest::addColumn<QString>("viewport");
    QTest::addColumn<bool>("covers");

    QTest::newRow("nothing said") << QString() << false;
    QTest::newRow("no fit") << QStringLiteral("width=device-width, initial-scale=1") << false;
    QTest::newRow("cover") << QStringLiteral("width=device-width, viewport-fit=cover") << true;
    QTest::newRow("cover first") << QStringLiteral("viewport-fit=cover,width=device-width") << true;
    QTest::newRow("spaced, any case")
        << QStringLiteral("width=device-width ,  Viewport-Fit = COVER ") << true;
    QTest::newRow("semicolons") << QStringLiteral("width=device-width; viewport-fit=cover;")
                                << true;
    QTest::newRow("contain") << QStringLiteral("viewport-fit=contain") << false;
    QTest::newRow("auto") << QStringLiteral("viewport-fit=auto") << false;
    // Not another setting that merely ends in the words.
    QTest::newRow("covered") << QStringLiteral("viewport-fit=covered") << false;
    QTest::newRow("prefixed") << QStringLiteral("x-viewport-fit=cover") << false;
}

void tst_enginemessages::coversCutout()
{
    QFETCH(QString, viewport);
    QFETCH(bool, covers);
    QCOMPARE(EngineMessages::coversCutout(viewport), covers);
}

// Do not track and JavaScript, as the preferences sailfish-browser's switches write.
void tst_enginemessages::contentPreferences()
{
    const QVariantList on = EngineMessages::contentPreferences(true, false);
    QCOMPARE(on.count(), 2);
    QCOMPARE(on.at(0).toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("privacy.donottrackheader.enabled"));
    QCOMPARE(on.at(0).toMap().value(QStringLiteral("value")), QVariant(true));
    QCOMPARE(on.at(1).toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("javascript.enabled"));
    QCOMPARE(on.at(1).toMap().value(QStringLiteral("value")), QVariant(false));
    const QVariantList off = EngineMessages::contentPreferences(false, true);
    QCOMPARE(off.at(0).toMap().value(QStringLiteral("value")), QVariant(false));
    QCOMPARE(off.at(1).toMap().value(QStringLiteral("value")), QVariant(true));
}

QTEST_GUILESS_MAIN(tst_enginemessages)
#include "tst_enginemessages.moc"
