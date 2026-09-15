// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#include "engine/EngineMessages.h"

#include <QtTest>

using Tuuli::EngineMessages;

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
};

void tst_enginemessages::constants()
{
    EngineMessages messages;
    QCOMPARE(messages.clearPrivateDataTopic(), QStringLiteral("clear-private-data"));
    QCOMPARE(messages.cookiesAndSiteDataPayload(), QStringLiteral("cookies-and-site-data"));
    QCOMPARE(messages.cachePayload(), QStringLiteral("cache"));
    QVERIFY(messages.faviconScript().contains(QStringLiteral("icon")));
    QVERIFY(messages.faviconScript().startsWith(QStringLiteral("(function")));
    QVERIFY(messages.themeColorScript().contains(QStringLiteral("theme-color")));
    QVERIFY(messages.themeColorScript().startsWith(QStringLiteral("(function")));
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

QTEST_GUILESS_MAIN(tst_enginemessages)
#include "tst_enginemessages.moc"
