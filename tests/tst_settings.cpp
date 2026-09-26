// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "settings/Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
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
    void trackingProtection();
    void readerStyle();
    void isAddress_data();
    void isAddress();
    void omnibarSources();
    void historySwitches();
    void blockNotificationRequests();
    void pageZoom();
    void quickAction();
    void quickActionBookmark();
    void quickActionIcon();
    void coverIconPath_data();
    void coverIconPath();
    void coverIconPathNamesTheSpeakers();
    void coverIconPathNamesEveryGlyph();
    void startPage();
    void retiresTheHomePage();
    void isSearchUrl_data();
    void isSearchUrl();
};

void tst_settings::defaults()
{
    QTemporaryDir dir;
    Settings settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.searchEngine(), Settings::defaultSearchEngine());
    QCOMPARE(settings.searchEngineIndex(), 0);
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
        QSignalSpy cutoutSpy(&settings, &Settings::cutoutGuardChanged);

        settings.setCutoutGuard(false);
        settings.setCutoutGuard(false);
        QCOMPARE(cutoutSpy.count(), 1);
        settings.setSearchEngine(QStringLiteral("startpage"));
    }
    Settings reloaded(path);
    QVERIFY(!reloaded.cutoutGuard());
    QCOMPARE(reloaded.searchEngine(), QStringLiteral("startpage"));
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

    // The lightning by default: the cover a reader who has not been to Settings gets.
    QCOMPARE(settings.coverStyle(), int(Settings::CoverLightning));

    settings.setCoverStyle(Settings::CoverLatestTab);
    QCOMPARE(settings.coverStyle(), int(Settings::CoverLatestTab));
    QCOMPARE(spy.count(), 1);

    // Setting what is already set says nothing.
    settings.setCoverStyle(Settings::CoverLatestTab);
    QCOMPARE(spy.count(), 1);

    // A value from outside the range is refused rather than stored: this comes from a
    // file a user can edit. 2 among them, the number the every-tab cover had.
    settings.setCoverStyle(7);
    settings.setCoverStyle(2);
    settings.setCoverStyle(-1);
    QCOMPARE(settings.coverStyle(), int(Settings::CoverLatestTab));
    QCOMPARE(spy.count(), 1);
    {
        Settings again(path);
        QCOMPARE(again.coverStyle(), int(Settings::CoverLatestTab));
    }

    settings.setCoverStyle(Settings::CoverLightning);
    QCOMPARE(spy.count(), 2);
    {
        Settings again(path);
        QCOMPARE(again.coverStyle(), int(Settings::CoverLightning));
    }

    // What the covers before the lightning left in the file. The icon alone was 0 and
    // the last tab 1, which the numbers still mean; every tab was 2, and reads back as
    // the default, as any value out of range does rather than as a cover that draws
    // nothing.
    const QList<QPair<int, int>> stored{
        {0, Settings::CoverLightning},
        {1, Settings::CoverLatestTab},
        {2, Settings::CoverLightning},
        {42, Settings::CoverLightning},
    };
    for (const QPair<int, int> &entry : stored) {
        {
            QSettings raw(path, QSettings::IniFormat);
            raw.setValue(QStringLiteral("coverStyle"), entry.first);
        }
        Settings again(path);
        QCOMPARE(again.coverStyle(), entry.second);
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
    // Shaped like a host with a port, but no port can be that: not an address QUrl
    // will take, so words to search for rather than nothing at all.
    QTest::newRow("port out of range")
        << QStringLiteral("example.org:99999")
        << QStringLiteral("https://www.qwant.com/?q=example.org%3A99999");
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

void tst_settings::trackingProtection()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Settings settings(path);
    QSignalSpy spy(&settings, &Settings::trackingProtectionChanged);

    // Standard by default, as in Firefox (docs/DECISIONS/0023-tracking-protection.md).
    QCOMPARE(settings.trackingProtection(), int(Settings::TrackingProtectionStandard));

    settings.setTrackingProtection(Settings::TrackingProtectionStrict);
    QCOMPARE(settings.trackingProtection(), int(Settings::TrackingProtectionStrict));
    QCOMPARE(spy.count(), 1);
    settings.setTrackingProtection(Settings::TrackingProtectionStrict);
    QCOMPARE(spy.count(), 1);

    // Refused rather than stored, as the cover's style is.
    settings.setTrackingProtection(3);
    settings.setTrackingProtection(-1);
    QCOMPARE(settings.trackingProtection(), int(Settings::TrackingProtectionStrict));
    QCOMPARE(spy.count(), 1);

    settings.setTrackingProtection(Settings::TrackingProtectionOff);
    QCOMPARE(spy.count(), 2);
    {
        Settings again(path);
        QCOMPARE(again.trackingProtection(), int(Settings::TrackingProtectionOff));
    }

    // Written by hand, out of range: the default, not a level the engine has no
    // preferences for.
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("trackingProtection"), 9);
    }
    {
        Settings again(path);
        QCOMPARE(again.trackingProtection(), int(Settings::TrackingProtectionStandard));
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

void tst_settings::isAddress_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("expected");

    QTest::newRow("host") << QStringLiteral("example.org") << true;
    QTest::newRow("host path") << QStringLiteral("example.org/path?q=1") << true;
    QTest::newRow("host port") << QStringLiteral("example.org:8443") << true;
    QTest::newRow("localhost") << QStringLiteral("localhost") << true;
    QTest::newRow("localhost port") << QStringLiteral("localhost:8080") << true;
    QTest::newRow("ipv4") << QStringLiteral("192.168.1.1") << true;
    QTest::newRow("https") << QStringLiteral("https://example.org/a?b=c#d") << true;
    QTest::newRow("about") << QStringLiteral("about:blank") << true;
    QTest::newRow("trimmed") << QStringLiteral("  example.org  ") << true;
    QTest::newRow("word") << QStringLiteral("sailfish") << false;
    QTest::newRow("words") << QStringLiteral("jolla phone 2026") << false;
    QTest::newRow("dotted words") << QStringLiteral("what is example.org") << false;
    QTest::newRow("unknown scheme") << QStringLiteral("gopher:hole") << false;
    QTest::newRow("port out of range") << QStringLiteral("example.org:99999") << false;
    QTest::newRow("empty") << QString() << false;
    QTest::newRow("blank") << QStringLiteral("   ") << false;
}

// An address exactly when urlForInput() does not make a search of it: the omnibar's
// "Go to" and Enter never disagree (docs/DECISIONS/0027-omnibar.md).
void tst_settings::isAddress()
{
    QFETCH(QString, input);
    QFETCH(bool, expected);
    QTemporaryDir dir;
    Settings settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.isAddress(input), expected);
    const QString url = settings.urlForInput(input);
    if (!input.trimmed().isEmpty()) {
        QCOMPARE(url == settings.searchUrl(input), !expected);
    }
}

// Every source of the address bar's suggestions is on until it is switched off, and
// each is kept under its own name.
void tst_settings::omnibarSources()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Settings settings(path);
        QVERIFY(settings.omnibarTabs());
        QVERIFY(settings.omnibarBookmarks());
        QVERIFY(settings.omnibarHistory());
        QVERIFY(settings.omnibarDownloads());

        QSignalSpy tabs(&settings, &Settings::omnibarTabsChanged);
        QSignalSpy bookmarks(&settings, &Settings::omnibarBookmarksChanged);
        QSignalSpy history(&settings, &Settings::omnibarHistoryChanged);
        QSignalSpy downloads(&settings, &Settings::omnibarDownloadsChanged);

        settings.setOmnibarTabs(true);
        QCOMPARE(tabs.count(), 0);
        settings.setOmnibarTabs(false);
        settings.setOmnibarTabs(false);
        QCOMPARE(tabs.count(), 1);
        settings.setOmnibarHistory(false);
        settings.setOmnibarHistory(false);
        QCOMPARE(history.count(), 1);
        settings.setOmnibarBookmarks(false);
        settings.setOmnibarBookmarks(true);
        QCOMPARE(bookmarks.count(), 2);
        settings.setOmnibarDownloads(false);
        QCOMPARE(downloads.count(), 1);
        // Each signal is its own flag's.
        QCOMPARE(tabs.count(), 1);
        QCOMPARE(history.count(), 1);
    }
    {
        Settings again(path);
        QVERIFY(!again.omnibarTabs());
        QVERIFY(again.omnibarBookmarks());
        QVERIFY(!again.omnibarHistory());
        QVERIFY(!again.omnibarDownloads());
    }
    QSettings raw(path, QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("omnibarTabs")).toBool(), false);
    QCOMPARE(raw.value(QStringLiteral("omnibarBookmarks")).toBool(), true);
    QCOMPARE(raw.value(QStringLiteral("omnibarHistory")).toBool(), false);
    QCOMPARE(raw.value(QStringLiteral("omnibarDownloads")).toBool(), false);
}

// The history is kept unless switched off, and cleared on closing only once switched on
// (docs/DECISIONS/0030-history-settings.md).
void tst_settings::historySwitches()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Settings settings(path);
        QVERIFY(settings.rememberHistory());
        QVERIFY(!settings.clearHistoryOnClose());
        QSignalSpy remember(&settings, &Settings::rememberHistoryChanged);
        QSignalSpy clear(&settings, &Settings::clearHistoryOnCloseChanged);

        settings.setRememberHistory(true);
        settings.setClearHistoryOnClose(false);
        QCOMPARE(remember.count(), 0);
        QCOMPARE(clear.count(), 0);
        settings.setRememberHistory(false);
        settings.setRememberHistory(false);
        settings.setClearHistoryOnClose(true);
        settings.setClearHistoryOnClose(true);
        QCOMPARE(remember.count(), 1);
        QCOMPARE(clear.count(), 1);
    }
    Settings again(path);
    QVERIFY(!again.rememberHistory());
    QVERIFY(again.clearHistoryOnClose());
    QSettings raw(path, QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("rememberHistory")).toBool(), false);
    QCOMPARE(raw.value(QStringLiteral("clearHistoryOnClose")).toBool(), true);
}

// Sites may ask to send notifications until new requests are blocked, as in Firefox
// (docs/DECISIONS/0033-web-notifications.md).
void tst_settings::blockNotificationRequests()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Settings settings(path);
        QVERIFY(!settings.blockNotificationRequests());
        QSignalSpy spy(&settings, &Settings::blockNotificationRequestsChanged);
        settings.setBlockNotificationRequests(false);
        QCOMPARE(spy.count(), 0);
        settings.setBlockNotificationRequests(true);
        settings.setBlockNotificationRequests(true);
        QCOMPARE(spy.count(), 1);
    }
    Settings again(path);
    QVERIFY(again.blockNotificationRequests());
    QSettings raw(path, QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("blockNotificationRequests")).toBool(), true);
}

// 1.75 of the screen's pixel ratio, in steps of a half.
void tst_settings::pageZoom()
{
    QCOMPARE(Settings::pageZoom(1.0), 2.0);
    QCOMPARE(Settings::pageZoom(1.5), 2.5);
    QCOMPARE(Settings::pageZoom(2.0), 3.5);
    QCOMPARE(Settings::pageZoom(2.25), 4.0);
}

// The cover's one quick action: Search unless changed, refused out of range and read
// back as Search when the file says something out of range
// (docs/DECISIONS/0029-quick-action.md).
void tst_settings::quickAction()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Settings settings(path);
    QSignalSpy spy(&settings, &Settings::quickActionChanged);

    QCOMPARE(settings.quickAction(), int(Settings::QuickActionSearch));
    // Stored as numbers, so the numbers may not move.
    QCOMPARE(int(Settings::QuickActionNone), 0);
    QCOMPARE(int(Settings::QuickActionSearch), 1);
    QCOMPARE(int(Settings::QuickActionBookmarks), 2);
    QCOMPARE(int(Settings::QuickActionBookmark), 3);
    QCOMPARE(int(Settings::QuickActionDownloads), 4);
    QCOMPARE(int(Settings::QuickActionHistory), 5);

    settings.setQuickAction(Settings::QuickActionHistory);
    settings.setQuickAction(Settings::QuickActionHistory);
    QCOMPARE(settings.quickAction(), int(Settings::QuickActionHistory));
    QCOMPARE(spy.count(), 1);
    settings.setQuickAction(Settings::QuickActionHistory + 1);
    settings.setQuickAction(-1);
    QCOMPARE(settings.quickAction(), int(Settings::QuickActionHistory));
    QCOMPARE(spy.count(), 1);
    settings.setQuickAction(Settings::QuickActionNone);
    QCOMPARE(spy.count(), 2);
    {
        Settings again(path);
        QCOMPARE(again.quickAction(), int(Settings::QuickActionNone));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        QCOMPARE(raw.value(QStringLiteral("quickAction")).toInt(), 0);
        raw.setValue(QStringLiteral("quickAction"), 12);
    }
    {
        Settings again(path);
        QCOMPARE(again.quickAction(), int(Settings::QuickActionSearch));
    }
}

// The bookmark is written whole, id, address and title in one go, with one signal.
void tst_settings::quickActionBookmark()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Settings settings(path);
        QSignalSpy spy(&settings, &Settings::quickActionBookmarkChanged);
        QCOMPARE(settings.quickActionBookmark(), 0);
        QVERIFY(settings.quickActionBookmarkUrl().isEmpty());
        QVERIFY(settings.quickActionBookmarkTitle().isEmpty());

        settings.setQuickActionBookmark(4, QStringLiteral("https://a.example/"),
                                        QStringLiteral("A"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.quickActionBookmark(), 4);
        QCOMPARE(settings.quickActionBookmarkUrl(), QStringLiteral("https://a.example/"));
        QCOMPARE(settings.quickActionBookmarkTitle(), QStringLiteral("A"));
        settings.setQuickActionBookmark(4, QStringLiteral("https://a.example/"),
                                        QStringLiteral("A"));
        QCOMPARE(spy.count(), 1);
        // A negative id is no bookmark's, and nothing is written.
        settings.setQuickActionBookmark(-2, QStringLiteral("https://b.example/"),
                                        QStringLiteral("B"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.quickActionBookmark(), 4);

        // Found again under a new id: the same address, and one signal.
        settings.setQuickActionBookmark(9, QStringLiteral("https://a.example/"),
                                        QStringLiteral("A"));
        QCOMPARE(spy.count(), 2);
        // Only the title changed is a change.
        settings.setQuickActionBookmark(9, QStringLiteral("https://a.example/"),
                                        QStringLiteral("Alpha"));
        QCOMPARE(spy.count(), 3);
        // The action is left as it was.
        QCOMPARE(settings.quickAction(), int(Settings::QuickActionSearch));
    }
    {
        Settings again(path);
        QCOMPARE(again.quickActionBookmark(), 9);
        QCOMPARE(again.quickActionBookmarkUrl(), QStringLiteral("https://a.example/"));
        QCOMPARE(again.quickActionBookmarkTitle(), QStringLiteral("Alpha"));

        // No bookmark is no address and no title either.
        QSignalSpy spy(&again, &Settings::quickActionBookmarkChanged);
        again.setQuickActionBookmark(0, QStringLiteral("https://a.example/"),
                                     QStringLiteral("Alpha"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(again.quickActionBookmark(), 0);
        QVERIFY(again.quickActionBookmarkUrl().isEmpty());
        QVERIFY(again.quickActionBookmarkTitle().isEmpty());
        again.setQuickActionBookmark(0, QString(), QString());
        QCOMPARE(spy.count(), 1);
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("quickActionBookmark"), -5);
    }
    Settings again(path);
    QCOMPARE(again.quickActionBookmark(), 0);
}

void tst_settings::quickActionIcon()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Settings settings(path);
    QSignalSpy spy(&settings, &Settings::quickActionIconChanged);

    // The star last: it is the bookmarks overview's own glyph.
    QCOMPARE(settings.quickActionIcons(),
             (QStringList{QStringLiteral("globe"), QStringLiteral("heart"), QStringLiteral("home"),
                          QStringLiteral("work"), QStringLiteral("news"), QStringLiteral("music"),
                          QStringLiteral("shop"), QStringLiteral("star")}));
    QCOMPARE(settings.quickActionIcon(), QStringLiteral("globe"));

    settings.setQuickActionIcon(QStringLiteral("music"));
    settings.setQuickActionIcon(QStringLiteral("music"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.quickActionIcon(), QStringLiteral("music"));
    settings.setQuickActionIcon(QStringLiteral("rocket"));
    settings.setQuickActionIcon(QString());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.quickActionIcon(), QStringLiteral("music"));
    {
        Settings again(path);
        QCOMPARE(again.quickActionIcon(), QStringLiteral("music"));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("quickActionIcon"), QStringLiteral("Music"));
    }
    Settings again(path);
    QCOMPARE(again.quickActionIcon(), QStringLiteral("globe"));
}

void tst_settings::coverIconPath_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<qreal>("size");
    QTest::addColumn<bool>("onDark");
    QTest::addColumn<QString>("expected");

    QTest::newRow("exact") << QStringLiteral("search") << qreal(48) << true
                           << QStringLiteral("art/cover/search-48-white.png");
    QTest::newRow("black") << QStringLiteral("star") << qreal(40) << false
                           << QStringLiteral("art/cover/star-40-black.png");
    QTest::newRow("down") << QStringLiteral("globe") << qreal(51.9) << true
                          << QStringLiteral("art/cover/globe-48-white.png");
    QTest::newRow("up") << QStringLiteral("globe") << qreal(52.1) << true
                        << QStringLiteral("art/cover/globe-56-white.png");
    // Halfway rounds up, as Math.round() does.
    QTest::newRow("halfway") << QStringLiteral("globe") << qreal(36) << false
                             << QStringLiteral("art/cover/globe-40-black.png");
    QTest::newRow("small") << QStringLiteral("history") << qreal(12) << true
                           << QStringLiteral("art/cover/history-32-white.png");
    QTest::newRow("large") << QStringLiteral("downloads") << qreal(200) << true
                           << QStringLiteral("art/cover/downloads-64-white.png");
    QTest::newRow("absurd") << QStringLiteral("home") << qreal(1e300) << false
                            << QStringLiteral("art/cover/home-64-black.png");
    QTest::newRow("negative") << QStringLiteral("home") << qreal(-8) << false
                              << QStringLiteral("art/cover/home-32-black.png");
}

void tst_settings::coverIconPath()
{
    QFETCH(QString, name);
    QFETCH(qreal, size);
    QFETCH(bool, onDark);
    QFETCH(QString, expected);
    QCOMPARE(Settings::coverIconPath(name, size, onDark), expected);
}

// The mute the cover already draws is found where coverIconPath() says, at every size
// and in both inks: the path is the one icons/render.sh writes.
void tst_settings::coverIconPathNamesTheSpeakers()
{
    const QDir source(QStringLiteral(SALAMA_SOURCE_DIR));
    for (const QString &name : {QStringLiteral("speaker-on"), QStringLiteral("speaker-mute")}) {
        for (int size = 32; size <= 64; size += 8) {
            for (const bool onDark : {true, false}) {
                const QString path = Settings::coverIconPath(name, size, onDark);
                QVERIFY2(QFileInfo::exists(source.absoluteFilePath(path)), qPrintable(path));
            }
        }
    }
}

// Every glyph the quick action can wear -- each kind's own, and each a bookmark's action
// can be given -- is where coverIconPath() says, at every size and in both inks, and
// the black is not the white: icons/render.sh draws the black set by rewriting the one
// colour the white is written in, and a glyph written in another spelling of white would
// come out white twice (docs/DECISIONS/0029-quick-action.md).
void tst_settings::coverIconPathNamesEveryGlyph()
{
    QTemporaryDir dir;
    Settings settings(dir.path() + QStringLiteral("/salama.conf"));
    const QDir source(QStringLiteral(SALAMA_SOURCE_DIR));
    const QStringList names = QStringList{QStringLiteral("search"), QStringLiteral("bookmarks"),
                                          QStringLiteral("downloads"), QStringLiteral("history")} +
                              settings.quickActionIcons();
    QCOMPARE(names.count(), 12);
    const auto contents = [&source](const QString &path) {
        QFile file(source.absoluteFilePath(path));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    for (const QString &name : names) {
        for (int size = 32; size <= 64; size += 8) {
            const QString white = Settings::coverIconPath(name, size, true);
            const QString black = Settings::coverIconPath(name, size, false);
            QVERIFY2(white.endsWith(QStringLiteral("-%1-white.png").arg(size)), qPrintable(white));
            QVERIFY2(black.endsWith(QStringLiteral("-%1-black.png").arg(size)), qPrintable(black));
            const QByteArray whiteFile = contents(white);
            const QByteArray blackFile = contents(black);
            QVERIFY2(!whiteFile.isEmpty(), qPrintable(white));
            QVERIFY2(!blackFile.isEmpty(), qPrintable(black));
            QVERIFY2(whiteFile != blackFile, qPrintable(black));
        }
    }
}

// The start page is Firefox's home: every section on, and not blank, until changed; a
// section keeps its switch while the page is blank (docs/DECISIONS/0032-start-page.md).
void tst_settings::startPage()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Settings settings(path);
        QVERIFY(!settings.startPageBlank());
        QVERIFY(settings.startPageTopSites());
        QVERIFY(settings.startPageBookmarks());
        QVERIFY(settings.startPageRecent());

        QSignalSpy spy(&settings, &Settings::startPageChanged);
        settings.setStartPageTopSites(true);
        settings.setStartPageBlank(false);
        QCOMPARE(spy.count(), 0);
        settings.setStartPageBlank(true);
        settings.setStartPageBlank(true);
        QCOMPARE(spy.count(), 1);
        settings.setStartPageTopSites(false);
        settings.setStartPageBookmarks(false);
        settings.setStartPageRecent(false);
        QCOMPARE(spy.count(), 4);
        settings.setStartPageRecent(true);
        QCOMPARE(spy.count(), 5);
    }
    Settings reloaded(path);
    QVERIFY(reloaded.startPageBlank());
    QVERIFY(!reloaded.startPageTopSites());
    QVERIFY(!reloaded.startPageBookmarks());
    QVERIFY(reloaded.startPageRecent());
}

// The home page an earlier release kept is taken out of the file: the start page took
// its place, and a key nothing reads is only a question for whoever opens the file.
void tst_settings::retiresTheHomePage()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        QSettings old(path, QSettings::IniFormat);
        old.setValue(QStringLiteral("homePage"), QStringLiteral("https://sailfishos.org/"));
        old.setValue(QStringLiteral("cutoutGuard"), false);
    }
    {
        Settings settings(path);
        QVERIFY(!settings.cutoutGuard());
    }
    QSettings file(path, QSettings::IniFormat);
    QVERIFY(!file.contains(QStringLiteral("homePage")));
    QVERIFY(file.contains(QStringLiteral("cutoutGuard")));
}

void tst_settings::isSearchUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("search");

    QTemporaryDir dir;
    Settings settings(dir.path() + QStringLiteral("/salama.conf"));
    for (int i = 0; i < settings.searchEngineKeys().count(); ++i) {
        settings.setSearchEngineIndex(i);
        const QString results = settings.searchUrl(QStringLiteral("sailfish os"));
        QTest::newRow(qPrintable(settings.searchEngineKeys().at(i))) << results << true;
    }
    // As the engines themselves hand the results on: with parameters of their own ahead
    // of the words, and with or without "www.".
    QTest::newRow("qwant, redirected")
        << QStringLiteral("https://qwant.com/?t=web&q=sailfish") << true;
    QTest::newRow("ecosia, redirected")
        << QStringLiteral("https://www.ecosia.org/search?method=index&q=sailfish") << true;
    QTest::newRow("qwant, front page") << QStringLiteral("https://www.qwant.com/") << false;
    QTest::newRow("ecosia, elsewhere")
        << QStringLiteral("https://www.ecosia.org/trees?q=sailfish") << false;
    QTest::newRow("another site's q")
        << QStringLiteral("https://example.org/search?q=sailfish") << false;
    QTest::newRow("no host") << QStringLiteral("about:blank") << false;
    QTest::newRow("empty") << QString() << false;
}

void tst_settings::isSearchUrl()
{
    QFETCH(QString, url);
    QFETCH(bool, search);
    QCOMPARE(Settings::isSearchUrl(url), search);
}

QTEST_GUILESS_MAIN(tst_settings)
#include "tst_settings.moc"
