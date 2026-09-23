// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "settings/Settings.h"

#include <QDir>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::Settings;

class tst_settings : public QObject
{
    Q_OBJECT

private slots:
    void defaults();
    void persistsValues();
    void searchEngineSelection();
    void searchUrl();
    void urlForInput_data();
    void urlForInput();
    void displayAddress_data();
    void displayAddress();
    void coverStyle();
    void liveTabLimit();
    void readerStyle();
};

void tst_settings::defaults()
{
    QTemporaryDir dir;
    Settings settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.homePage(), Settings::defaultHomePage());
    QCOMPARE(settings.searchEngine(), Settings::defaultSearchEngine());
    QCOMPARE(settings.searchEngineIndex(), 0);
    QVERIFY(!settings.desktopMode());
    // On unless it is turned off: a camera cutout over the first line of a page is
    // not a design decision (docs/DECISIONS/0013-screen-cutout.md).
    QVERIFY(settings.cutoutGuard());
    QCOMPARE(settings.searchEngineNames().count(), settings.searchEngineKeys().count());
    QCOMPARE(settings.searchEngineNames().first(), QStringLiteral("Qwant"));
    QVERIFY(settings.searchEngineNames().contains(QStringLiteral("Ecosia")));
    // Removed by choice, and the list is the whole set on offer.
    QVERIFY(!settings.searchEngineKeys().contains(QStringLiteral("google")));
    QVERIFY(!settings.searchEngineKeys().contains(QStringLiteral("bing")));
    QVERIFY(!settings.searchEngineKeys().contains(QStringLiteral("duckduckgo")));
    QVERIFY(!settings.searchEngineKeys().contains(QStringLiteral("wikipedia")));
    QVERIFY(settings.searchEngineKeys().contains(QStringLiteral("startpage")));
}

void tst_settings::persistsValues()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Settings settings(path);
        QSignalSpy homeSpy(&settings, &Settings::homePageChanged);
        QSignalSpy desktopSpy(&settings, &Settings::desktopModeChanged);
        QSignalSpy cutoutSpy(&settings, &Settings::cutoutGuardChanged);

        settings.setHomePage(QStringLiteral("  https://sailfishos.org/  "));
        settings.setHomePage(QStringLiteral("https://sailfishos.org/"));
        QCOMPARE(homeSpy.count(), 1);
        QCOMPARE(settings.homePage(), QStringLiteral("https://sailfishos.org/"));

        settings.setDesktopMode(true);
        settings.setDesktopMode(true);
        QCOMPARE(desktopSpy.count(), 1);

        settings.setCutoutGuard(false);
        settings.setCutoutGuard(false);
        QCOMPARE(cutoutSpy.count(), 1);
        settings.setSearchEngine(QStringLiteral("startpage"));
    }
    Settings reloaded(path);
    QCOMPARE(reloaded.homePage(), QStringLiteral("https://sailfishos.org/"));
    QVERIFY(reloaded.desktopMode());
    QVERIFY(!reloaded.cutoutGuard());
    QCOMPARE(reloaded.searchEngine(), QStringLiteral("startpage"));

    reloaded.setHomePage(QStringLiteral("   "));
    QCOMPARE(reloaded.homePage(), Settings::defaultHomePage());
}

void tst_settings::searchEngineSelection()
{
    QTemporaryDir dir;
    Settings settings(dir.path() + QStringLiteral("/salama.conf"));
    QSignalSpy spy(&settings, &Settings::searchEngineChanged);

    settings.setSearchEngine(QStringLiteral("nonsense"));
    QCOMPARE(spy.count(), 0);
    settings.setSearchEngineIndex(-1);
    settings.setSearchEngineIndex(99);
    QCOMPARE(spy.count(), 0);

    settings.setSearchEngineIndex(2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.searchEngine(), QStringLiteral("startpage"));
    QCOMPARE(settings.searchEngineIndex(), 2);
    settings.setSearchEngine(QStringLiteral("startpage"));
    QCOMPARE(spy.count(), 1);
}

void tst_settings::coverStyle()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Settings settings(path);
    QSignalSpy spy(&settings, &Settings::coverStyleChanged);

    // Every tab by default: the cover a reader who has not been to Settings gets.
    QCOMPARE(settings.coverStyle(), int(Settings::CoverEveryTab));

    settings.setCoverStyle(Settings::CoverIconOnly);
    QCOMPARE(settings.coverStyle(), int(Settings::CoverIconOnly));
    QCOMPARE(spy.count(), 1);

    // Setting what is already set says nothing.
    settings.setCoverStyle(Settings::CoverIconOnly);
    QCOMPARE(spy.count(), 1);

    // A value from outside the range is refused rather than stored: this comes from a
    // file a user can edit.
    settings.setCoverStyle(7);
    settings.setCoverStyle(-1);
    QCOMPARE(settings.coverStyle(), int(Settings::CoverIconOnly));
    QCOMPARE(spy.count(), 1);

    settings.setCoverStyle(Settings::CoverLatestTab);
    QCOMPARE(spy.count(), 2);
    {
        Settings again(path);
        QCOMPARE(again.coverStyle(), int(Settings::CoverLatestTab));
    }

    // One written by hand, out of range: read back as the default, not as a cover that
    // draws nothing.
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("coverStyle"), 42);
    }
    {
        Settings again(path);
        QCOMPARE(again.coverStyle(), int(Settings::CoverEveryTab));
    }
}

void tst_settings::searchUrl()
{
    QTemporaryDir dir;
    Settings settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.searchUrl(QStringLiteral("sailfish os")),
             QStringLiteral("https://www.qwant.com/?q=sailfish%20os"));
    settings.setSearchEngine(QStringLiteral("ecosia"));
    QCOMPARE(settings.searchUrl(QStringLiteral(" a&b ")),
             QStringLiteral("https://www.ecosia.org/search?q=a%26b"));
}

void tst_settings::urlForInput_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("blank") << QStringLiteral("   ") << QString();
    QTest::newRow("https") << QStringLiteral("https://example.org/a?b=c#d")
                           << QStringLiteral("https://example.org/a?b=c#d");
    QTest::newRow("http") << QStringLiteral("http://example.org")
                          << QStringLiteral("http://example.org");
    QTest::newRow("about") << QStringLiteral("about:blank") << QStringLiteral("about:blank");
    QTest::newRow("host") << QStringLiteral("example.org") << QStringLiteral("https://example.org");
    QTest::newRow("host path") << QStringLiteral("example.org/path?q=1")
                               << QStringLiteral("https://example.org/path?q=1");
    QTest::newRow("host port") << QStringLiteral("example.org:8443")
                               << QStringLiteral("https://example.org:8443");
    QTest::newRow("localhost") << QStringLiteral("localhost:8080")
                               << QStringLiteral("http://localhost:8080");
    QTest::newRow("ipv4") << QStringLiteral("192.168.1.1/admin")
                          << QStringLiteral("http://192.168.1.1/admin");
    QTest::newRow("trimmed") << QStringLiteral("  example.org  ")
                             << QStringLiteral("https://example.org");
    QTest::newRow("word") << QStringLiteral("sailfish")
                          << QStringLiteral("https://www.qwant.com/?q=sailfish");
    QTest::newRow("words") << QStringLiteral("jolla phone 2026")
                           << QStringLiteral("https://www.qwant.com/?q=jolla%20phone%202026");
    QTest::newRow("dotted words") << QStringLiteral("what is example.org")
                                  << QStringLiteral(
                                         "https://www.qwant.com/?q=what%20is%20example.org");
    QTest::newRow("question") << QStringLiteral("how? really")
                              << QStringLiteral("https://www.qwant.com/?q=how%3F%20really");
}

void tst_settings::urlForInput()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QTemporaryDir dir;
    Settings settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.urlForInput(input), expected);
}

void tst_settings::displayAddress_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expected");

    QTest::newRow("www") << QStringLiteral("https://www.bellard.org/")
                         << QStringLiteral("bellard.org");
    QTest::newRow("path and query")
        << QStringLiteral("https://example.org/a/b?c=d#e") << QStringLiteral("example.org");
    QTest::newRow("subdomain kept") << QStringLiteral("https://en.wikipedia.org/wiki/Sailfish")
                                    << QStringLiteral("en.wikipedia.org");
    // "www" only counts as the prefix of the host, never in the middle of a name.
    QTest::newRow("wwwsomething") << QStringLiteral("https://wwwhat.example/")
                                  << QStringLiteral("wwwhat.example");
    QTest::newRow("http") << QStringLiteral("http://example.org/") << QStringLiteral("example.org");
    // The port is left off: 80 and 443 say nothing, and the rest are noise here.
    QTest::newRow("port") << QStringLiteral("http://localhost:8080/app")
                          << QStringLiteral("localhost");
    QTest::newRow("default port") << QStringLiteral("https://example.org:443/")
                                  << QStringLiteral("example.org");
    QTest::newRow("ipv4") << QStringLiteral("http://192.168.1.1/admin")
                          << QStringLiteral("192.168.1.1");
    // Nothing to shorten: shown as it is rather than emptied.
    QTest::newRow("about") << QStringLiteral("about:blank") << QStringLiteral("about:blank");
    QTest::newRow("file") << QStringLiteral("file:///home/user/page.html")
                          << QStringLiteral("file:///home/user/page.html");
    QTest::newRow("empty") << QString() << QString();
}

void tst_settings::displayAddress()
{
    QFETCH(QString, url);
    QFETCH(QString, expected);
    QCOMPARE(Settings::displayAddress(url), expected);
}

void tst_settings::liveTabLimit()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Settings settings(path);
    QSignalSpy spy(&settings, &Settings::liveTabLimitChanged);

    // Five by default, as Jolla's browser keeps; the choices are what the combo
    // offers, with 0 standing for all of them.
    QCOMPARE(settings.liveTabLimit(), Settings::defaultLiveTabLimit());
    QCOMPARE(settings.liveTabLimit(), 5);
    QCOMPARE(settings.liveTabLimitChoices(), (QVariantList{3, 5, 10, 0}));
    QCOMPARE(settings.liveTabLimitIndex(), 1);

    settings.setLiveTabLimitIndex(3);
    QCOMPARE(settings.liveTabLimit(), 0);
    QCOMPARE(spy.count(), 1);
    settings.setLiveTabLimitIndex(3);
    settings.setLiveTabLimitIndex(4);
    settings.setLiveTabLimitIndex(-1);
    QCOMPARE(spy.count(), 1);
    settings.setLiveTabLimitIndex(0);
    QCOMPARE(settings.liveTabLimit(), 3);
    {
        Settings again(path);
        QCOMPARE(again.liveTabLimit(), 3);
    }

    // A number written by hand that is not on offer reads back as the default.
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("liveTabLimit"), 7);
    }
    {
        Settings again(path);
        QCOMPARE(again.liveTabLimit(), 5);
    }
}

// How the reader view sets an article: the ambience's colours, sans-serif and Firefox's
// middle text size until they are changed, and each one refused out of range and read
// back as the default when the file says something out of range.
void tst_settings::readerStyle()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Settings settings(path);
        QCOMPARE(settings.readerColors(), int(Settings::ReaderAmbience));
        QCOMPARE(settings.readerTypeface(), int(Settings::ReaderSansSerif));
        QCOMPARE(settings.readerTextSize(), int(Settings::ReaderTextSizeDefault));
        QCOMPARE(int(Settings::ReaderTextSizeDefault), 5);

        QSignalSpy colors(&settings, &Settings::readerColorsChanged);
        QSignalSpy typeface(&settings, &Settings::readerTypefaceChanged);
        QSignalSpy size(&settings, &Settings::readerTextSizeChanged);

        settings.setReaderColors(Settings::ReaderSepia);
        settings.setReaderColors(Settings::ReaderSepia);
        settings.setReaderColors(Settings::ReaderDark + 1);
        settings.setReaderColors(-1);
        QCOMPARE(settings.readerColors(), int(Settings::ReaderSepia));
        QCOMPARE(colors.count(), 1);

        settings.setReaderTypeface(Settings::ReaderSerif);
        settings.setReaderTypeface(Settings::ReaderSerif);
        settings.setReaderTypeface(2);
        settings.setReaderTypeface(-1);
        QCOMPARE(settings.readerTypeface(), int(Settings::ReaderSerif));
        QCOMPARE(typeface.count(), 1);

        settings.setReaderTextSize(Settings::ReaderTextSizeMax);
        settings.setReaderTextSize(Settings::ReaderTextSizeMax);
        settings.setReaderTextSize(Settings::ReaderTextSizeMax + 1);
        settings.setReaderTextSize(Settings::ReaderTextSizeMin - 1);
        QCOMPARE(settings.readerTextSize(), int(Settings::ReaderTextSizeMax));
        QCOMPARE(size.count(), 1);
        settings.setReaderTextSize(Settings::ReaderTextSizeMin);
        QCOMPARE(size.count(), 2);
    }
    {
        Settings again(path);
        QCOMPARE(again.readerColors(), int(Settings::ReaderSepia));
        QCOMPARE(again.readerTypeface(), int(Settings::ReaderSerif));
        QCOMPARE(again.readerTextSize(), int(Settings::ReaderTextSizeMin));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("readerColors"), 9);
        raw.setValue(QStringLiteral("readerTypeface"), 5);
        raw.setValue(QStringLiteral("readerTextSize"), 40);
    }
    {
        Settings again(path);
        QCOMPARE(again.readerColors(), int(Settings::ReaderAmbience));
        QCOMPARE(again.readerTypeface(), int(Settings::ReaderSansSerif));
        QCOMPARE(again.readerTextSize(), int(Settings::ReaderTextSizeDefault));
    }
}

QTEST_GUILESS_MAIN(tst_settings)
#include "tst_settings.moc"
