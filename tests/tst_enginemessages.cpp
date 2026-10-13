// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/EngineMessages.h"
#include "settings/DohSettings.h"
#include "settings/PrivacySettings.h"
#include "settings/Settings.h"
#include "settings/SitePermissionSettings.h"

#include <QtTest>

using Salama::DohSettings;
using Salama::EngineMessages;
using Salama::PrivacySettings;
using Salama::Settings;
using Salama::SitePermissionSettings;

namespace {

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

QVariantMap valuesOf(const QVariantList &list)
{
    QVariantMap values;
    for (const QVariant &entry : list) {
        const QVariantMap preference = entry.toMap();
        values.insert(preference.value(QStringLiteral("name")).toString(),
                      preference.value(QStringLiteral("value")));
    }
    return values;
}

QStringList namesOf(const QVariantList &list)
{
    QStringList names;
    for (const QVariant &entry : list) {
        names.append(entry.toMap().value(QStringLiteral("name")).toString());
    }
    return names;
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

// Manual split for empty list: Qt 5.6 lacks Qt::SkipEmptyParts, host Qt deprecates
// QString::SkipEmptyParts.
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
    void httpsOnlyPreferences();
    void dohPreferences_data();
    void dohPreferences();
};

void tst_enginemessages::constants()
{
    EngineMessages messages;
    QCOMPARE(messages.clearPrivateDataTopic(), QStringLiteral("clear-private-data"));
    QCOMPARE(messages.cookiesAndSiteDataPayload(), QStringLiteral("cookies-and-site-data"));
    QCOMPARE(messages.cachePayload(), QStringLiteral("cache"));
    // Engine wraps script in function (embedhelper.js: new content.Function(script)), so script
    // is function body and must return; bare expression gives callback undefined.
    QVERIFY(messages.faviconScript().contains(QStringLiteral("icon")));
    QVERIFY(messages.faviconScript().contains(QStringLiteral("return ")));
    QVERIFY(!messages.faviconScript().contains(QStringLiteral("function")));
    QVERIFY(messages.themeColorScript().contains(QStringLiteral("theme-color")));
    QVERIFY(messages.themeColorScript().contains(QStringLiteral("return ")));
    QVERIFY(!messages.themeColorScript().contains(QStringLiteral("function")));
    QVERIFY(messages.viewportScript().contains(QStringLiteral("viewport")));
    QVERIFY(messages.viewportScript().contains(QStringLiteral("return ")));
    QVERIFY(!messages.viewportScript().contains(QStringLiteral("function")));
    QCOMPARE(messages.findMessage(), QStringLiteral("embedui:find"));
    QCOMPARE(messages.findResultMessage(), QStringLiteral("embed:find"));
}

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
    QCOMPARE(offered.value(QStringLiteral("host")).toString(), QStringLiteral("find.example"));
    QCOMPARE(offered.count(), 3);

    const QVariantMap unplaced = EngineMessages::searchOffered(
        QVariantMap{{QStringLiteral("engine"), engine},
                    {QStringLiteral("url"), QStringLiteral("about:blank")}});
    QCOMPARE(unplaced.value(QStringLiteral("host")).toString(), QStringLiteral("cdn.example"));
    QCOMPARE(EngineMessages::searchOffered(QVariantMap{{QStringLiteral("engine"), engine}})
                 .value(QStringLiteral("host"))
                 .toString(),
             QStringLiteral("cdn.example"));

    for (const QVariant &data : {QVariant(), QVariant(QStringLiteral("text")), QVariant(42),
                                 QVariant(QVariantMap{{QStringLiteral("engine"), 7}})}) {
        const QVariantMap nothing = EngineMessages::searchOffered(data);
        QCOMPARE(nothing.count(), 3);
        QVERIFY(nothing.value(QStringLiteral("title")).toString().isEmpty());
        QVERIFY(nothing.value(QStringLiteral("href")).toString().isEmpty());
    }
}

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
    // Link text one line: textContent keeps page line breaks.
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
    QTest::newRow("javascript") << link(QStringLiteral("javascript:void(0)"),
                                        QStringLiteral("More"))
                                << QString() << QString() << QString() << QStringLiteral("More")
                                << QString();
    QTest::newRow("no address") << link(QString(), QStringLiteral("More")) << QString() << QString()
                                << QString() << QStringLiteral("More") << QString();

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
    QTest::newRow("image") << QVariantMap{{QStringLiteral("types"),
                                           QStringList{QStringLiteral("image")}},
                                          {QStringLiteral("mediaURL"),
                                           QStringLiteral("https://cdn.example/photos/ridge.jpg")},
                                          {QStringLiteral("linkTitle"), QStringLiteral("Dawn")}}
                           << QString() << QString()
                           << QStringLiteral("https://cdn.example/photos/ridge.jpg")
                           << QStringLiteral("Dawn")
                           << QStringLiteral("cdn.example/photos/ridge.jpg");
    QTest::newRow("data image")
        << QVariantMap{{QStringLiteral("types"), QStringList{QStringLiteral("image")}},
                       {QStringLiteral("mediaURL"), QStringLiteral("data:image/png;base64,AAAA")}}
        << QString() << QString() << QString() << QString() << QString();
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

    const QVariantMap end = messages.findRequest(QString(), false, false);
    QVERIFY(end.contains(QStringLiteral("text")));
    QVERIFY(end.value(QStringLiteral("text")).toString().isEmpty());
}

// Page answers {"r": nsITypeAheadFind result}. Found and found-wrapped = present; non-number
// = not.
void tst_enginemessages::findFound_data()
{
    QTest::addColumn<QVariant>("data");
    QTest::addColumn<bool>("expected");
    const QString r = QStringLiteral("r");
    // JSON numbers -> double on device Qt 5.6.
    QTest::newRow("found") << QVariant(QVariantMap{{r, 0.0}}) << true;
    QTest::newRow("not found") << QVariant(QVariantMap{{r, 1.0}}) << false;
    QTest::newRow("wrapped") << QVariant(QVariantMap{{r, 2.0}}) << true;
    QTest::newRow("pending") << QVariant(QVariantMap{{r, 3.0}}) << false;
    QTest::newRow("found int") << QVariant(QVariantMap{{r, 0}}) << true;
    QTest::newRow("wrapped int") << QVariant(QVariantMap{{r, 2}}) << true;
    QTest::newRow("not found int") << QVariant(QVariantMap{{r, 1}}) << false;
    // Qt 5.15+ reads JSON whole number as qlonglong.
    QTest::newRow("found longlong") << QVariant(QVariantMap{{r, 0LL}}) << true;
    QTest::newRow("wrapped longlong") << QVariant(QVariantMap{{r, 2LL}}) << true;
    QTest::newRow("pending longlong") << QVariant(QVariantMap{{r, 3LL}}) << false;
    QTest::newRow("wrapped uint") << QVariant(QVariantMap{{r, 2U}}) << true;
    QTest::newRow("not found uint") << QVariant(QVariantMap{{r, 1U}}) << false;
    QTest::newRow("found ulonglong") << QVariant(QVariantMap{{r, 0ULL}}) << true;
    QTest::newRow("not found ulonglong") << QVariant(QVariantMap{{r, 1ULL}}) << false;
    QTest::newRow("unknown") << QVariant(QVariantMap{{r, 7.0}}) << false;
    QTest::newRow("fraction") << QVariant(QVariantMap{{r, 0.5}}) << false;
    QTest::newRow("missing") << QVariant(QVariantMap{}) << false;
    QTest::newRow("null") << QVariant(QVariantMap{{r, QVariant()}}) << false;
    // QVariant reads 0 from these = FIND_FOUND.
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

void tst_enginemessages::themeColor_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<QString>("expected");
    const QString blue = QStringLiteral("#123456");
    QTest::newRow("hex") << blue << blue;
    QTest::newRow("padded") << QStringLiteral("  #123456  ") << blue;
    QTest::newRow("short hex") << QStringLiteral("#abc") << QStringLiteral("#aabbcc");
    // CSS 8-digit hex = alpha last; QColor = alpha first (#123456ff -> #3456ff otherwise).
    QTest::newRow("hex with alpha") << QStringLiteral("#123456ff") << blue;
    QTest::newRow("named") << QStringLiteral("darkslateblue") << QStringLiteral("#483d8b");
    QTest::newRow("rgb") << QStringLiteral("rgb(18, 52, 86)") << blue;
    QTest::newRow("rgb spaces") << QStringLiteral("rgb(18 52 86)") << blue;
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

// Every level writes every pref, same order and type, so Strict -> Off leaves no Strict
// residue; setPreference() picks setter by value type, engine rejects wrong type.
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

    // Off = stock engine (nothing classified, bounce tracking watched not acted on), except
    // cookies: reader's choice, Block cross-site here (others: cookiesAtOffAreTheReadersChoice()).
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

    QCOMPARE(trackingValues(3), standard);
    QCOMPARE(trackingValues(-1), standard);
}

// Engine's own feature names and required order: blocking list ends with exception-only
// features; annotation has none.
void tst_enginemessages::trackingProtectionFeatures()
{
    // kFeatures in gecko-dev toolkit/components/content-classifier/
    // ContentClassifierService.cpp, minus two test-only ones.
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

// Cookie choice applies only when protection Off: Standard/Strict use Firefox's. Only one
// of twelve prefs that differs.
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
    QCOMPARE(trackingValues(off, 7).value(cookies), QVariant(1));
    QCOMPARE(trackingValues(off, -1).value(cookies), QVariant(1));
    QCOMPARE(trackingValues(off, 2).value(cookies).userType(), int(QMetaType::Int));

    for (const int level : {int(PrivacySettings::TrackingProtectionStandard),
                            int(PrivacySettings::TrackingProtectionStrict)}) {
        for (const int choice : {0, 1, 2}) {
            QCOMPARE(trackingValues(level, choice).value(cookies), QVariant(5));
        }
    }
    QVariantMap allowAll = trackingValues(off, SitePermissionSettings::CookiesAllowAll);
    QVariantMap blockAll = trackingValues(off, SitePermissionSettings::CookiesBlockAll);
    allowAll.remove(cookies);
    blockAll.remove(cookies);
    QCOMPARE(allowAll, blockAll);
}

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
    QTest::newRow("covered") << QStringLiteral("viewport-fit=covered") << false;
    QTest::newRow("prefixed") << QStringLiteral("x-viewport-fit=cover") << false;
}

void tst_enginemessages::coversCutout()
{
    QFETCH(QString, viewport);
    QFETCH(bool, covers);
    QCOMPARE(EngineMessages::coversCutout(viewport), covers);
}

// GPC sent only when both prefs on; Do Not Track (replaced) always off.
void tst_enginemessages::contentPreferences()
{
    const QStringList names{QStringLiteral("privacy.donottrackheader.enabled"),
                            QStringLiteral("privacy.globalprivacycontrol.functionality.enabled"),
                            QStringLiteral("privacy.globalprivacycontrol.enabled"),
                            QStringLiteral("javascript.enabled")};
    const QVariantList on = EngineMessages::contentPreferences(true, false);
    QCOMPARE(namesOf(on), names);
    const QVariantMap onValues = valuesOf(on);
    QCOMPARE(onValues.value(names.at(0)), QVariant(false));
    QCOMPARE(onValues.value(names.at(1)), QVariant(true));
    QCOMPARE(onValues.value(names.at(2)), QVariant(true));
    QCOMPARE(onValues.value(names.at(3)), QVariant(false));
    const QVariantList off = EngineMessages::contentPreferences(false, true);
    QCOMPARE(namesOf(off), names);
    const QVariantMap offValues = valuesOf(off);
    QCOMPARE(offValues.value(names.at(0)), QVariant(false));
    QCOMPARE(offValues.value(names.at(1)), QVariant(true));
    QCOMPARE(offValues.value(names.at(2)), QVariant(false));
    QCOMPARE(offValues.value(names.at(3)), QVariant(true));
    // Booleans so engine sets as booleans.
    for (const QVariant &value : onValues) {
        QCOMPARE(value.userType(), int(QMetaType::Bool));
    }
}

void tst_enginemessages::httpsOnlyPreferences()
{
    const QStringList names{QStringLiteral("dom.security.https_only_mode"),
                            QStringLiteral("dom.security.https_first")};
    const QVariantList on = EngineMessages::httpsOnlyPreferences(true);
    QCOMPARE(namesOf(on), names);
    QCOMPARE(valuesOf(on).value(names.at(0)), QVariant(true));
    QCOMPARE(valuesOf(on).value(names.at(1)), QVariant(true));
    const QVariantList off = EngineMessages::httpsOnlyPreferences(false);
    QCOMPARE(namesOf(off), names);
    QCOMPARE(valuesOf(off).value(names.at(0)), QVariant(false));
    QCOMPARE(valuesOf(off).value(names.at(1)), QVariant(true));
}

// Level -> GeckoView resolver mode, else Off. Provider and exceptions before mode so level
// never starts on stale provider. Mode int, rest text, as engine stores.
void tst_enginemessages::dohPreferences_data()
{
    QTest::addColumn<int>("protection");
    QTest::addColumn<int>("mode");
    QTest::newRow("off") << int(DohSettings::ProtectionOff) << 5;
    QTest::newRow("increased") << int(DohSettings::ProtectionIncreased) << 2;
    QTest::newRow("max") << int(DohSettings::ProtectionMax) << 3;
    QTest::newRow("out of range") << 9 << 5;
    QTest::newRow("negative") << -1 << 5;
}

void tst_enginemessages::dohPreferences()
{
    QFETCH(int, protection);
    QFETCH(int, mode);
    const QString provider = QStringLiteral("https://dns.example.org/dns-query");
    const QVariantList list = EngineMessages::dohPreferences(
        protection, provider,
        {QStringLiteral("intranet.example.com"), QStringLiteral("router.local")});
    QCOMPARE(namesOf(list), (QStringList{QStringLiteral("network.trr.uri"),
                                         QStringLiteral("network.trr.excluded-domains"),
                                         QStringLiteral("network.trr.mode")}));
    const QVariantMap values = valuesOf(list);
    QCOMPARE(values.value(QStringLiteral("network.trr.mode")), QVariant(mode));
    QCOMPARE(values.value(QStringLiteral("network.trr.mode")).userType(), int(QMetaType::Int));
    QCOMPARE(values.value(QStringLiteral("network.trr.uri")), QVariant(provider));
    QCOMPARE(values.value(QStringLiteral("network.trr.excluded-domains")),
             QVariant(QStringLiteral("intranet.example.com,router.local")));
    QCOMPARE(values.value(QStringLiteral("network.trr.excluded-domains")).userType(),
             int(QMetaType::QString));
    const QVariantMap none = valuesOf(EngineMessages::dohPreferences(protection, provider, {}));
    QCOMPARE(none.value(QStringLiteral("network.trr.excluded-domains")), QVariant(QString()));
}

QTEST_GUILESS_MAIN(tst_enginemessages)
#include "tst_enginemessages.moc"
