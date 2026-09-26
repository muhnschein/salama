// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "settings/CoverSettings.h"
#include "settings/PrivacySettings.h"
#include "settings/ReaderSettings.h"
#include "settings/SearchSettings.h"
#include "settings/Settings.h"
#include "settings/StartPageSettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::CoverSettings;
using Salama::PrivacySettings;
using Salama::ReaderSettings;
using Salama::SearchSettings;
using Salama::Settings;
using Salama::StartPageSettings;

namespace {

// The settings file and every section over it, as Core keeps them.
struct Sections
{
    explicit Sections(const QString &path)
        : file(path, QSettings::IniFormat)
    {
    }

    QSettings file;
    Settings general{file};
    SearchSettings search{file};
    ReaderSettings reader{file};
    CoverSettings cover{file};
    PrivacySettings privacy{file};
    StartPageSettings startPage{file};
};

} // namespace

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
    void tutorialShown();
    void isSearchUrl_data();
    void isSearchUrl();
};

void tst_settings::defaults()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.search.engine(), SearchSettings::defaultEngine());
    QCOMPARE(settings.search.engineIndex(), 0);
    // On unless it is turned off: a camera cutout over the first line of a page is
    // not a design decision (docs/DECISIONS/0013-screen-cutout.md).
    QVERIFY(settings.general.cutoutGuard());
    QCOMPARE(settings.search.engineNames().count(), settings.search.engineKeys().count());
    QCOMPARE(settings.search.engineNames().first(), QStringLiteral("Qwant"));
    QVERIFY(settings.search.engineNames().contains(QStringLiteral("Ecosia")));
    // Removed by choice, and the list is the whole set on offer.
    QVERIFY(!settings.search.engineKeys().contains(QStringLiteral("google")));
    QVERIFY(!settings.search.engineKeys().contains(QStringLiteral("bing")));
    QVERIFY(!settings.search.engineKeys().contains(QStringLiteral("duckduckgo")));
    QVERIFY(!settings.search.engineKeys().contains(QStringLiteral("wikipedia")));
    QVERIFY(settings.search.engineKeys().contains(QStringLiteral("startpage")));
}

void tst_settings::persistsValues()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy cutoutSpy(&settings.general, &Settings::cutoutGuardChanged);

        settings.general.setCutoutGuard(false);
        settings.general.setCutoutGuard(false);
        QCOMPARE(cutoutSpy.count(), 1);
        settings.search.setEngine(QStringLiteral("startpage"));
    }
    Sections reloaded(path);
    QVERIFY(!reloaded.general.cutoutGuard());
    QCOMPARE(reloaded.search.engine(), QStringLiteral("startpage"));
}

void tst_settings::searchEngineSelection()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QSignalSpy spy(&settings.search, &SearchSettings::engineChanged);

    settings.search.setEngine(QStringLiteral("nonsense"));
    QCOMPARE(spy.count(), 0);
    settings.search.setEngineIndex(-1);
    settings.search.setEngineIndex(99);
    QCOMPARE(spy.count(), 0);

    settings.search.setEngineIndex(2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.search.engine(), QStringLiteral("startpage"));
    QCOMPARE(settings.search.engineIndex(), 2);
    settings.search.setEngine(QStringLiteral("startpage"));
    QCOMPARE(spy.count(), 1);
}

void tst_settings::coverStyle()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Sections settings(path);
    QSignalSpy spy(&settings.cover, &CoverSettings::styleChanged);

    // The lightning by default: the cover a reader who has not been to Settings gets.
    QCOMPARE(settings.cover.style(), int(CoverSettings::Lightning));

    settings.cover.setStyle(CoverSettings::LatestTab);
    QCOMPARE(settings.cover.style(), int(CoverSettings::LatestTab));
    QCOMPARE(spy.count(), 1);

    // Setting what is already set says nothing.
    settings.cover.setStyle(CoverSettings::LatestTab);
    QCOMPARE(spy.count(), 1);

    // A value from outside the range is refused rather than stored: this comes from a
    // file a user can edit. 2 among them, the number the every-tab cover had.
    settings.cover.setStyle(7);
    settings.cover.setStyle(2);
    settings.cover.setStyle(-1);
    QCOMPARE(settings.cover.style(), int(CoverSettings::LatestTab));
    QCOMPARE(spy.count(), 1);
    {
        Sections again(path);
        QCOMPARE(again.cover.style(), int(CoverSettings::LatestTab));
    }

    settings.cover.setStyle(CoverSettings::Lightning);
    QCOMPARE(spy.count(), 2);
    {
        Sections again(path);
        QCOMPARE(again.cover.style(), int(CoverSettings::Lightning));
    }

    // What the covers before the lightning left in the file. The icon alone was 0 and
    // the last tab 1, which the numbers still mean; every tab was 2, and reads back as
    // the default, as any value out of range does rather than as a cover that draws
    // nothing.
    const QList<QPair<int, int>> stored{
        {0, CoverSettings::Lightning},
        {1, CoverSettings::LatestTab},
        {2, CoverSettings::Lightning},
        {42, CoverSettings::Lightning},
    };
    for (const QPair<int, int> &entry : stored) {
        {
            QSettings raw(path, QSettings::IniFormat);
            raw.setValue(QStringLiteral("coverStyle"), entry.first);
        }
        Sections again(path);
        QCOMPARE(again.cover.style(), entry.second);
    }
}

void tst_settings::searchUrl()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.search.searchUrl(QStringLiteral("sailfish os")),
             QStringLiteral("https://www.qwant.com/?q=sailfish%20os"));
    settings.search.setEngine(QStringLiteral("ecosia"));
    QCOMPARE(settings.search.searchUrl(QStringLiteral(" a&b ")),
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
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.search.urlForInput(input), expected);
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
    QCOMPARE(SearchSettings::displayAddress(url), expected);
}

void tst_settings::trackingProtection()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Sections settings(path);
    QSignalSpy spy(&settings.privacy, &PrivacySettings::trackingProtectionChanged);

    // Standard by default, as in Firefox (docs/DECISIONS/0023-tracking-protection.md).
    QCOMPARE(settings.privacy.trackingProtection(),
             int(PrivacySettings::TrackingProtectionStandard));

    settings.privacy.setTrackingProtection(PrivacySettings::TrackingProtectionStrict);
    QCOMPARE(settings.privacy.trackingProtection(), int(PrivacySettings::TrackingProtectionStrict));
    QCOMPARE(spy.count(), 1);
    settings.privacy.setTrackingProtection(PrivacySettings::TrackingProtectionStrict);
    QCOMPARE(spy.count(), 1);

    // Refused rather than stored, as the cover's style is.
    settings.privacy.setTrackingProtection(3);
    settings.privacy.setTrackingProtection(-1);
    QCOMPARE(settings.privacy.trackingProtection(), int(PrivacySettings::TrackingProtectionStrict));
    QCOMPARE(spy.count(), 1);

    settings.privacy.setTrackingProtection(PrivacySettings::TrackingProtectionOff);
    QCOMPARE(spy.count(), 2);
    {
        Sections again(path);
        QCOMPARE(again.privacy.trackingProtection(), int(PrivacySettings::TrackingProtectionOff));
    }

    // Written by hand, out of range: the default, not a level the engine has no
    // preferences for.
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("trackingProtection"), 9);
    }
    {
        Sections again(path);
        QCOMPARE(again.privacy.trackingProtection(),
                 int(PrivacySettings::TrackingProtectionStandard));
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
        Sections settings(path);
        QCOMPARE(settings.reader.colors(), int(ReaderSettings::Ambience));
        QCOMPARE(settings.reader.typeface(), int(ReaderSettings::SansSerif));
        QCOMPARE(settings.reader.textSize(), int(ReaderSettings::TextSizeDefault));
        QCOMPARE(int(ReaderSettings::TextSizeDefault), 5);

        QSignalSpy colors(&settings.reader, &ReaderSettings::colorsChanged);
        QSignalSpy typeface(&settings.reader, &ReaderSettings::typefaceChanged);
        QSignalSpy size(&settings.reader, &ReaderSettings::textSizeChanged);

        settings.reader.setColors(ReaderSettings::Sepia);
        settings.reader.setColors(ReaderSettings::Sepia);
        settings.reader.setColors(ReaderSettings::Dark + 1);
        settings.reader.setColors(-1);
        QCOMPARE(settings.reader.colors(), int(ReaderSettings::Sepia));
        QCOMPARE(colors.count(), 1);

        settings.reader.setTypeface(ReaderSettings::Serif);
        settings.reader.setTypeface(ReaderSettings::Serif);
        settings.reader.setTypeface(2);
        settings.reader.setTypeface(-1);
        QCOMPARE(settings.reader.typeface(), int(ReaderSettings::Serif));
        QCOMPARE(typeface.count(), 1);

        settings.reader.setTextSize(ReaderSettings::TextSizeMax);
        settings.reader.setTextSize(ReaderSettings::TextSizeMax);
        settings.reader.setTextSize(ReaderSettings::TextSizeMax + 1);
        settings.reader.setTextSize(ReaderSettings::TextSizeMin - 1);
        QCOMPARE(settings.reader.textSize(), int(ReaderSettings::TextSizeMax));
        QCOMPARE(size.count(), 1);
        settings.reader.setTextSize(ReaderSettings::TextSizeMin);
        QCOMPARE(size.count(), 2);
    }
    {
        Sections again(path);
        QCOMPARE(again.reader.colors(), int(ReaderSettings::Sepia));
        QCOMPARE(again.reader.typeface(), int(ReaderSettings::Serif));
        QCOMPARE(again.reader.textSize(), int(ReaderSettings::TextSizeMin));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("readerColors"), 9);
        raw.setValue(QStringLiteral("readerTypeface"), 5);
        raw.setValue(QStringLiteral("readerTextSize"), 40);
    }
    {
        Sections again(path);
        QCOMPARE(again.reader.colors(), int(ReaderSettings::Ambience));
        QCOMPARE(again.reader.typeface(), int(ReaderSettings::SansSerif));
        QCOMPARE(again.reader.textSize(), int(ReaderSettings::TextSizeDefault));
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
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.search.isAddress(input), expected);
    const QString url = settings.search.urlForInput(input);
    if (!input.trimmed().isEmpty()) {
        QCOMPARE(url == settings.search.searchUrl(input), !expected);
    }
}

// Every source of the address bar's suggestions is on until it is switched off, and
// each is kept under its own name.
void tst_settings::omnibarSources()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Sections settings(path);
        QVERIFY(settings.search.omnibarTabs());
        QVERIFY(settings.search.omnibarBookmarks());
        QVERIFY(settings.search.omnibarHistory());
        QVERIFY(settings.search.omnibarDownloads());

        QSignalSpy tabs(&settings.search, &SearchSettings::omnibarTabsChanged);
        QSignalSpy bookmarks(&settings.search, &SearchSettings::omnibarBookmarksChanged);
        QSignalSpy history(&settings.search, &SearchSettings::omnibarHistoryChanged);
        QSignalSpy downloads(&settings.search, &SearchSettings::omnibarDownloadsChanged);

        settings.search.setOmnibarTabs(true);
        QCOMPARE(tabs.count(), 0);
        settings.search.setOmnibarTabs(false);
        settings.search.setOmnibarTabs(false);
        QCOMPARE(tabs.count(), 1);
        settings.search.setOmnibarHistory(false);
        settings.search.setOmnibarHistory(false);
        QCOMPARE(history.count(), 1);
        settings.search.setOmnibarBookmarks(false);
        settings.search.setOmnibarBookmarks(true);
        QCOMPARE(bookmarks.count(), 2);
        settings.search.setOmnibarDownloads(false);
        QCOMPARE(downloads.count(), 1);
        // Each signal is its own flag's.
        QCOMPARE(tabs.count(), 1);
        QCOMPARE(history.count(), 1);
    }
    {
        Sections again(path);
        QVERIFY(!again.search.omnibarTabs());
        QVERIFY(again.search.omnibarBookmarks());
        QVERIFY(!again.search.omnibarHistory());
        QVERIFY(!again.search.omnibarDownloads());
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
        Sections settings(path);
        QVERIFY(settings.privacy.rememberHistory());
        QVERIFY(!settings.privacy.clearHistoryOnClose());
        QSignalSpy remember(&settings.privacy, &PrivacySettings::rememberHistoryChanged);
        QSignalSpy clear(&settings.privacy, &PrivacySettings::clearHistoryOnCloseChanged);

        settings.privacy.setRememberHistory(true);
        settings.privacy.setClearHistoryOnClose(false);
        QCOMPARE(remember.count(), 0);
        QCOMPARE(clear.count(), 0);
        settings.privacy.setRememberHistory(false);
        settings.privacy.setRememberHistory(false);
        settings.privacy.setClearHistoryOnClose(true);
        settings.privacy.setClearHistoryOnClose(true);
        QCOMPARE(remember.count(), 1);
        QCOMPARE(clear.count(), 1);
    }
    Sections again(path);
    QVERIFY(!again.privacy.rememberHistory());
    QVERIFY(again.privacy.clearHistoryOnClose());
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
        Sections settings(path);
        QVERIFY(!settings.privacy.blockNotificationRequests());
        QSignalSpy spy(&settings.privacy, &PrivacySettings::blockNotificationRequestsChanged);
        settings.privacy.setBlockNotificationRequests(false);
        QCOMPARE(spy.count(), 0);
        settings.privacy.setBlockNotificationRequests(true);
        settings.privacy.setBlockNotificationRequests(true);
        QCOMPARE(spy.count(), 1);
    }
    Sections again(path);
    QVERIFY(again.privacy.blockNotificationRequests());
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
    Sections settings(path);
    QSignalSpy spy(&settings.cover, &CoverSettings::quickActionChanged);

    QCOMPARE(settings.cover.quickAction(), int(CoverSettings::QuickActionSearch));
    // Stored as numbers, so the numbers may not move.
    QCOMPARE(int(CoverSettings::QuickActionNone), 0);
    QCOMPARE(int(CoverSettings::QuickActionSearch), 1);
    QCOMPARE(int(CoverSettings::QuickActionBookmarks), 2);
    QCOMPARE(int(CoverSettings::QuickActionBookmark), 3);
    QCOMPARE(int(CoverSettings::QuickActionDownloads), 4);
    QCOMPARE(int(CoverSettings::QuickActionHistory), 5);

    settings.cover.setQuickAction(CoverSettings::QuickActionHistory);
    settings.cover.setQuickAction(CoverSettings::QuickActionHistory);
    QCOMPARE(settings.cover.quickAction(), int(CoverSettings::QuickActionHistory));
    QCOMPARE(spy.count(), 1);
    settings.cover.setQuickAction(CoverSettings::QuickActionHistory + 1);
    settings.cover.setQuickAction(-1);
    QCOMPARE(settings.cover.quickAction(), int(CoverSettings::QuickActionHistory));
    QCOMPARE(spy.count(), 1);
    settings.cover.setQuickAction(CoverSettings::QuickActionNone);
    QCOMPARE(spy.count(), 2);
    {
        Sections again(path);
        QCOMPARE(again.cover.quickAction(), int(CoverSettings::QuickActionNone));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        QCOMPARE(raw.value(QStringLiteral("quickAction")).toInt(), 0);
        raw.setValue(QStringLiteral("quickAction"), 12);
    }
    {
        Sections again(path);
        QCOMPARE(again.cover.quickAction(), int(CoverSettings::QuickActionSearch));
    }
}

// The bookmark is written whole, id, address and title in one go, with one signal.
void tst_settings::quickActionBookmark()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Sections settings(path);
        QSignalSpy spy(&settings.cover, &CoverSettings::quickActionBookmarkChanged);
        QCOMPARE(settings.cover.quickActionBookmark(), 0);
        QVERIFY(settings.cover.quickActionBookmarkUrl().isEmpty());
        QVERIFY(settings.cover.quickActionBookmarkTitle().isEmpty());

        settings.cover.setQuickActionBookmark(4, QStringLiteral("https://a.example/"),
                                              QStringLiteral("A"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.cover.quickActionBookmark(), 4);
        QCOMPARE(settings.cover.quickActionBookmarkUrl(), QStringLiteral("https://a.example/"));
        QCOMPARE(settings.cover.quickActionBookmarkTitle(), QStringLiteral("A"));
        settings.cover.setQuickActionBookmark(4, QStringLiteral("https://a.example/"),
                                              QStringLiteral("A"));
        QCOMPARE(spy.count(), 1);
        // A negative id is no bookmark's, and nothing is written.
        settings.cover.setQuickActionBookmark(-2, QStringLiteral("https://b.example/"),
                                              QStringLiteral("B"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.cover.quickActionBookmark(), 4);

        // Found again under a new id: the same address, and one signal.
        settings.cover.setQuickActionBookmark(9, QStringLiteral("https://a.example/"),
                                              QStringLiteral("A"));
        QCOMPARE(spy.count(), 2);
        // Only the title changed is a change.
        settings.cover.setQuickActionBookmark(9, QStringLiteral("https://a.example/"),
                                              QStringLiteral("Alpha"));
        QCOMPARE(spy.count(), 3);
        // The action is left as it was.
        QCOMPARE(settings.cover.quickAction(), int(CoverSettings::QuickActionSearch));
    }
    {
        Sections again(path);
        QCOMPARE(again.cover.quickActionBookmark(), 9);
        QCOMPARE(again.cover.quickActionBookmarkUrl(), QStringLiteral("https://a.example/"));
        QCOMPARE(again.cover.quickActionBookmarkTitle(), QStringLiteral("Alpha"));

        // No bookmark is no address and no title either.
        QSignalSpy spy(&again.cover, &CoverSettings::quickActionBookmarkChanged);
        again.cover.setQuickActionBookmark(0, QStringLiteral("https://a.example/"),
                                           QStringLiteral("Alpha"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(again.cover.quickActionBookmark(), 0);
        QVERIFY(again.cover.quickActionBookmarkUrl().isEmpty());
        QVERIFY(again.cover.quickActionBookmarkTitle().isEmpty());
        again.cover.setQuickActionBookmark(0, QString(), QString());
        QCOMPARE(spy.count(), 1);
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("quickActionBookmark"), -5);
    }
    Sections again(path);
    QCOMPARE(again.cover.quickActionBookmark(), 0);
}

void tst_settings::quickActionIcon()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Sections settings(path);
    QSignalSpy spy(&settings.cover, &CoverSettings::quickActionIconChanged);

    // The star last: it is the bookmarks overview's own glyph.
    QCOMPARE(settings.cover.quickActionIcons(),
             (QStringList{QStringLiteral("globe"), QStringLiteral("heart"), QStringLiteral("home"),
                          QStringLiteral("work"), QStringLiteral("news"), QStringLiteral("music"),
                          QStringLiteral("shop"), QStringLiteral("star")}));
    QCOMPARE(settings.cover.quickActionIcon(), QStringLiteral("globe"));

    settings.cover.setQuickActionIcon(QStringLiteral("music"));
    settings.cover.setQuickActionIcon(QStringLiteral("music"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.cover.quickActionIcon(), QStringLiteral("music"));
    settings.cover.setQuickActionIcon(QStringLiteral("rocket"));
    settings.cover.setQuickActionIcon(QString());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.cover.quickActionIcon(), QStringLiteral("music"));
    {
        Sections again(path);
        QCOMPARE(again.cover.quickActionIcon(), QStringLiteral("music"));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("quickActionIcon"), QStringLiteral("Music"));
    }
    Sections again(path);
    QCOMPARE(again.cover.quickActionIcon(), QStringLiteral("globe"));
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
    QCOMPARE(CoverSettings::iconPath(name, size, onDark), expected);
}

// The mute the cover already draws is found where coverIconPath() says, at every size
// and in both inks: the path is the one icons/render.sh writes.
void tst_settings::coverIconPathNamesTheSpeakers()
{
    const QDir source(QStringLiteral(SALAMA_SOURCE_DIR));
    for (const QString &name : {QStringLiteral("speaker-on"), QStringLiteral("speaker-mute")}) {
        for (int size = 32; size <= 64; size += 8) {
            for (const bool onDark : {true, false}) {
                const QString path = CoverSettings::iconPath(name, size, onDark);
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
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    const QDir source(QStringLiteral(SALAMA_SOURCE_DIR));
    const QStringList names = QStringList{QStringLiteral("search"), QStringLiteral("bookmarks"),
                                          QStringLiteral("downloads"), QStringLiteral("history")} +
                              settings.cover.quickActionIcons();
    QCOMPARE(names.count(), 12);
    const auto contents = [&source](const QString &path) {
        QFile file(source.absoluteFilePath(path));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    for (const QString &name : names) {
        for (int size = 32; size <= 64; size += 8) {
            const QString white = CoverSettings::iconPath(name, size, true);
            const QString black = CoverSettings::iconPath(name, size, false);
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
        Sections settings(path);
        QVERIFY(!settings.startPage.blank());
        QVERIFY(settings.startPage.topSites());
        QVERIFY(settings.startPage.bookmarks());
        QVERIFY(settings.startPage.recent());

        QSignalSpy spy(&settings.startPage, &StartPageSettings::changed);
        settings.startPage.setTopSites(true);
        settings.startPage.setBlank(false);
        QCOMPARE(spy.count(), 0);
        settings.startPage.setBlank(true);
        settings.startPage.setBlank(true);
        QCOMPARE(spy.count(), 1);
        settings.startPage.setTopSites(false);
        settings.startPage.setBookmarks(false);
        settings.startPage.setRecent(false);
        QCOMPARE(spy.count(), 4);
        settings.startPage.setRecent(true);
        QCOMPARE(spy.count(), 5);
    }
    Sections reloaded(path);
    QVERIFY(reloaded.startPage.blank());
    QVERIFY(!reloaded.startPage.topSites());
    QVERIFY(!reloaded.startPage.bookmarks());
    QVERIFY(reloaded.startPage.recent());
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
        Sections settings(path);
        QVERIFY(!settings.general.cutoutGuard());
    }
    QSettings file(path, QSettings::IniFormat);
    QVERIFY(!file.contains(QStringLiteral("homePage")));
    QVERIFY(file.contains(QStringLiteral("cutoutGuard")));
}

// The tutorial comes up by itself until it has been shown once, and stays shown
// (docs/DECISIONS/0034-tutorial.md).
void tst_settings::tutorialShown()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QVERIFY(!settings.general.tutorialShown());
        QSignalSpy spy(&settings.general, &Settings::tutorialShownChanged);
        settings.general.setTutorialShown(false);
        QCOMPARE(spy.count(), 0);
        settings.general.setTutorialShown(true);
        settings.general.setTutorialShown(true);
        QCOMPARE(spy.count(), 1);
    }
    Sections reloaded(path);
    QVERIFY(reloaded.general.tutorialShown());
}

void tst_settings::isSearchUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("search");

    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    for (int i = 0; i < settings.search.engineKeys().count(); ++i) {
        settings.search.setEngineIndex(i);
        const QString results = settings.search.searchUrl(QStringLiteral("sailfish os"));
        QTest::newRow(qPrintable(settings.search.engineKeys().at(i))) << results << true;
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
    QCOMPARE(SearchSettings::isSearchUrl(url), search);
}

QTEST_GUILESS_MAIN(tst_settings)
#include "tst_settings.moc"
