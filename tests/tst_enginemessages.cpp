// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/EngineMessages.h"
#include "settings/Settings.h"

#include <QtTest>

using Salama::EngineMessages;
using Salama::Settings;

namespace {

QVariantMap trackingValues(int level)
{
    QVariantMap values;
    for (const QVariant &entry : EngineMessages::trackingProtectionPreferences(level)) {
        const QVariantMap preference = entry.toMap();
        values.insert(preference.value(QStringLiteral("name")).toString(),
                      preference.value(QStringLiteral("value")));
    }
    return values;
}

QStringList trackingNames(int level)
{
    QStringList names;
    for (const QVariant &entry : EngineMessages::trackingProtectionPreferences(level)) {
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
    void findFound_data();
    void findFound();
    void trackingProtectionNamesTheSamePreferences();
    void trackingProtectionLevels();
    void trackingProtectionFeatures();
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
    // Find in page: what embedhelper.js listens for, and what it answers on.
    QCOMPARE(messages.findMessage(), QStringLiteral("embedui:find"));
    QCOMPARE(messages.findResultMessage(), QStringLiteral("embed:find"));
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
    QStringList names = trackingNames(Settings::TrackingProtectionStandard);
    QCOMPARE(names.count(), 12);
    QCOMPARE(names.removeDuplicates(), 0);
    QCOMPARE(trackingNames(Settings::TrackingProtectionOff), names);
    QCOMPARE(trackingNames(Settings::TrackingProtectionStrict), names);

    for (const QVariant &entry :
         EngineMessages::trackingProtectionPreferences(Settings::TrackingProtectionStandard)) {
        QCOMPARE(entry.toMap().count(), 2);
    }
    const QVariantMap off = trackingValues(Settings::TrackingProtectionOff);
    const QVariantMap standard = trackingValues(Settings::TrackingProtectionStandard);
    const QVariantMap strict = trackingValues(Settings::TrackingProtectionStrict);
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

    // Off is the engine as it comes: every cookie accepted, nothing classified,
    // bounce tracking watched and never acted on.
    const QVariantMap off = trackingValues(Settings::TrackingProtectionOff);
    QCOMPARE(off.value(cookies), QVariant(0));
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
    const QVariantMap standard = trackingValues(Settings::TrackingProtectionStandard);
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
    const QVariantMap strict = trackingValues(Settings::TrackingProtectionStrict);
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

    for (int level :
         {int(Settings::TrackingProtectionStandard), int(Settings::TrackingProtectionStrict)}) {
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
                 level == Settings::TrackingProtectionStrict);
    }
}

QTEST_GUILESS_MAIN(tst_enginemessages)
#include "tst_enginemessages.moc"
