// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "settings/CoverSettings.h"
#include "settings/DohSettings.h"
#include "settings/PrivacySettings.h"
#include "settings/ReaderSettings.h"
#include "settings/SearchSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsSections.h"
#include "settings/StartPageSettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::CoverSettings;
using Salama::DohSettings;
using Salama::PrivacySettings;
using Salama::ReaderSettings;
using Salama::SearchSettings;
using Salama::Settings;
using Salama::StartPageSettings;
using Sections = Salama::SettingsSections;

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
    void trackingProtection();
    void readerStyle();
    void readerColorsOfAnEarlierRelease();
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
    void websiteColors();
    void isSearchUrl_data();
    void isSearchUrl();
    void notchGuard();
    void notchGuardOfAnEarlierRelease_data();
    void notchGuardOfAnEarlierRelease();
    void fixedToolbar();
    void linkPreview();
    void contentSwitches();
    void globalPrivacyControlTakesDoNotTracksPlace_data();
    void globalPrivacyControlTakesDoNotTracksPlace();
    void httpsOnly();
    void dohProtection();
    void dohProvider();
    void dohProviderProblem_data();
    void dohProviderProblem();
    void dohDomainOf_data();
    void dohDomainOf();
    void dohExceptions();
};

void tst_settings::defaults()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.search()->engine(), SearchSettings::defaultEngine());
    QCOMPARE(settings.search()->engineIndex(), 0);
    // Automatic by default, as sailfish-browser: cutout over page's first line isn't design.
    QCOMPARE(settings.general()->notchGuard(), int(Settings::NotchGuardAutomatic));
    QVERIFY(settings.general()->cutoutGuard());
    QVERIFY(!settings.general()->fixedToolbar());
    QVERIFY(settings.general()->linkPreview());
    QVERIFY(!settings.privacy()->globalPrivacyControl());
    QVERIFY(settings.privacy()->javascript());
    QVERIFY(!settings.privacy()->httpsOnly());
    QCOMPARE(settings.doh()->protection(), int(DohSettings::ProtectionOff));
    QCOMPARE(settings.doh()->provider(), DohSettings::defaultProvider());
    QVERIFY(settings.doh()->exceptions().isEmpty());
    QCOMPARE(settings.searchEngines()->engineNames().count(),
             settings.searchEngines()->engineKeys().count());
    QCOMPARE(settings.searchEngines()->engineNames().first(), QStringLiteral("Qwant"));
    QVERIFY(settings.searchEngines()->engineNames().contains(QStringLiteral("Ecosia")));
    QVERIFY(!settings.searchEngines()->engineKeys().contains(QStringLiteral("google")));
    QVERIFY(!settings.searchEngines()->engineKeys().contains(QStringLiteral("bing")));
    QVERIFY(!settings.searchEngines()->engineKeys().contains(QStringLiteral("duckduckgo")));
    QVERIFY(!settings.searchEngines()->engineKeys().contains(QStringLiteral("wikipedia")));
    QVERIFY(settings.searchEngines()->engineKeys().contains(QStringLiteral("startpage")));
}

void tst_settings::persistsValues()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy guardSpy(settings.general(), &Settings::notchGuardChanged);

        settings.general()->setNotchGuard(Settings::NotchGuardDisabled);
        settings.general()->setNotchGuard(Settings::NotchGuardDisabled);
        QCOMPARE(guardSpy.count(), 1);
        settings.search()->setEngine(QStringLiteral("startpage"));
    }
    Sections reloaded(path);
    QCOMPARE(reloaded.general()->notchGuard(), int(Settings::NotchGuardDisabled));
    QVERIFY(!reloaded.general()->cutoutGuard());
    QCOMPARE(reloaded.search()->engine(), QStringLiteral("startpage"));
}

void tst_settings::searchEngineSelection()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QSignalSpy spy(settings.search(), &SearchSettings::engineChanged);

    settings.search()->setEngine(QStringLiteral("nonsense"));
    QCOMPARE(spy.count(), 0);
    settings.search()->setEngineIndex(-1);
    settings.search()->setEngineIndex(99);
    QCOMPARE(spy.count(), 0);

    settings.search()->setEngineIndex(2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.search()->engine(), QStringLiteral("startpage"));
    QCOMPARE(settings.search()->engineIndex(), 2);
    settings.search()->setEngine(QStringLiteral("startpage"));
    QCOMPARE(spy.count(), 1);
}

void tst_settings::searchUrl()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.search()->searchUrl(QStringLiteral("sailfish os")),
             QStringLiteral("https://www.qwant.com/?q=sailfish%20os"));
    settings.search()->setEngine(QStringLiteral("ecosia"));
    QCOMPARE(settings.search()->searchUrl(QStringLiteral(" a&b ")),
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
    // Host:port shape with impossible port: QUrl rejects, so search words, not empty.
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
    QCOMPARE(settings.search()->urlForInput(input), expected);
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
    QTest::newRow("wwwsomething") << QStringLiteral("https://wwwhat.example/")
                                  << QStringLiteral("wwwhat.example");
    QTest::newRow("http") << QStringLiteral("http://example.org/") << QStringLiteral("example.org");
    QTest::newRow("port") << QStringLiteral("http://localhost:8080/app")
                          << QStringLiteral("localhost");
    QTest::newRow("default port") << QStringLiteral("https://example.org:443/")
                                  << QStringLiteral("example.org");
    QTest::newRow("ipv4") << QStringLiteral("http://192.168.1.1/admin")
                          << QStringLiteral("192.168.1.1");
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
    QSignalSpy spy(settings.privacy(), &PrivacySettings::trackingProtectionChanged);

    QCOMPARE(settings.privacy()->trackingProtection(),
             int(PrivacySettings::TrackingProtectionStandard));

    settings.privacy()->setTrackingProtection(PrivacySettings::TrackingProtectionStrict);
    QCOMPARE(settings.privacy()->trackingProtection(),
             int(PrivacySettings::TrackingProtectionStrict));
    QCOMPARE(spy.count(), 1);
    settings.privacy()->setTrackingProtection(PrivacySettings::TrackingProtectionStrict);
    QCOMPARE(spy.count(), 1);

    settings.privacy()->setTrackingProtection(3);
    settings.privacy()->setTrackingProtection(-1);
    QCOMPARE(settings.privacy()->trackingProtection(),
             int(PrivacySettings::TrackingProtectionStrict));
    QCOMPARE(spy.count(), 1);

    settings.privacy()->setTrackingProtection(PrivacySettings::TrackingProtectionOff);
    QCOMPARE(spy.count(), 2);
    {
        Sections again(path);
        QCOMPARE(again.privacy()->trackingProtection(),
                 int(PrivacySettings::TrackingProtectionOff));
    }

    // Hand-written out-of-range: default, not level engine has no prefs for.
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("trackingProtection"), 9);
    }
    {
        Sections again(path);
        QCOMPARE(again.privacy()->trackingProtection(),
                 int(PrivacySettings::TrackingProtectionStandard));
    }
}

void tst_settings::readerStyle()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Sections settings(path);
        QCOMPARE(settings.reader()->colors(), int(ReaderSettings::Ambience));
        QCOMPARE(settings.reader()->typeface(), int(ReaderSettings::SansSerif));
        QCOMPARE(settings.reader()->textSize(), int(ReaderSettings::TextSizeDefault));
        QCOMPARE(int(ReaderSettings::TextSizeDefault), 5);

        QSignalSpy colors(settings.reader(), &ReaderSettings::colorsChanged);
        QSignalSpy typeface(settings.reader(), &ReaderSettings::typefaceChanged);
        QSignalSpy size(settings.reader(), &ReaderSettings::textSizeChanged);

        settings.reader()->setColors(ReaderSettings::Sepia);
        settings.reader()->setColors(ReaderSettings::Sepia);
        settings.reader()->setColors(ReaderSettings::Ambience + 1);
        settings.reader()->setColors(-1);
        QCOMPARE(settings.reader()->colors(), int(ReaderSettings::Sepia));
        QCOMPARE(colors.count(), 1);

        settings.reader()->setTypeface(ReaderSettings::Serif);
        settings.reader()->setTypeface(ReaderSettings::Serif);
        settings.reader()->setTypeface(2);
        settings.reader()->setTypeface(-1);
        QCOMPARE(settings.reader()->typeface(), int(ReaderSettings::Serif));
        QCOMPARE(typeface.count(), 1);

        settings.reader()->setTextSize(ReaderSettings::TextSizeMax);
        settings.reader()->setTextSize(ReaderSettings::TextSizeMax);
        settings.reader()->setTextSize(ReaderSettings::TextSizeMax + 1);
        settings.reader()->setTextSize(ReaderSettings::TextSizeMin - 1);
        QCOMPARE(settings.reader()->textSize(), int(ReaderSettings::TextSizeMax));
        QCOMPARE(size.count(), 1);
        settings.reader()->setTextSize(ReaderSettings::TextSizeMin);
        QCOMPARE(size.count(), 2);
    }
    {
        Sections again(path);
        QCOMPARE(again.reader()->colors(), int(ReaderSettings::Sepia));
        QCOMPARE(again.reader()->typeface(), int(ReaderSettings::Serif));
        QCOMPARE(again.reader()->textSize(), int(ReaderSettings::TextSizeMin));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("readerColorScheme"), 9);
        raw.setValue(QStringLiteral("readerTypeface"), 5);
        raw.setValue(QStringLiteral("readerTextSize"), 40);
    }
    {
        Sections again(path);
        QCOMPARE(again.reader()->colors(), int(ReaderSettings::Ambience));
        QCOMPARE(again.reader()->typeface(), int(ReaderSettings::SansSerif));
        QCOMPARE(again.reader()->textSize(), int(ReaderSettings::TextSizeDefault));
    }
}

// Old format: 0 = theme per ambience -> now Ambience look (also follows ambience); chosen
// theme kept. Old key removed so later Automatic sticks.
void tst_settings::readerColorsOfAnEarlierRelease()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    const auto earlier = [&path](int colors) {
        QSettings raw(path, QSettings::IniFormat);
        raw.clear();
        raw.setValue(QStringLiteral("readerColors"), colors);
    };
    const auto kept = [&path]() {
        return QSettings(path, QSettings::IniFormat).contains(QStringLiteral("readerColors"));
    };

    earlier(ReaderSettings::Automatic);
    {
        Sections settings(path);
        QCOMPARE(settings.reader()->colors(), int(ReaderSettings::Ambience));
        settings.reader()->setColors(ReaderSettings::Automatic);
    }
    QVERIFY(!kept());
    {
        Sections again(path);
        QCOMPARE(again.reader()->colors(), int(ReaderSettings::Automatic));
    }

    earlier(ReaderSettings::Sepia);
    {
        Sections settings(path);
        QCOMPARE(settings.reader()->colors(), int(ReaderSettings::Sepia));
    }
    QVERIFY(!kept());
    {
        Sections again(path);
        QCOMPARE(again.reader()->colors(), int(ReaderSettings::Sepia));
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

// Address iff urlForInput() doesn't search it: omnibar "Go to" and Enter agree.
void tst_settings::isAddress()
{
    QFETCH(QString, input);
    QFETCH(bool, expected);
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.search()->isAddress(input), expected);
    const QString url = settings.search()->urlForInput(input);
    if (!input.trimmed().isEmpty()) {
        QCOMPARE(url == settings.search()->searchUrl(input), !expected);
    }
}

void tst_settings::omnibarSources()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Sections settings(path);
        QVERIFY(settings.search()->omnibarTabs());
        QVERIFY(settings.search()->omnibarBookmarks());
        QVERIFY(settings.search()->omnibarHistory());
        QVERIFY(settings.search()->omnibarDownloads());

        QSignalSpy tabs(settings.search(), &SearchSettings::omnibarTabsChanged);
        QSignalSpy bookmarks(settings.search(), &SearchSettings::omnibarBookmarksChanged);
        QSignalSpy history(settings.search(), &SearchSettings::omnibarHistoryChanged);
        QSignalSpy downloads(settings.search(), &SearchSettings::omnibarDownloadsChanged);

        settings.search()->setOmnibarTabs(true);
        QCOMPARE(tabs.count(), 0);
        settings.search()->setOmnibarTabs(false);
        settings.search()->setOmnibarTabs(false);
        QCOMPARE(tabs.count(), 1);
        settings.search()->setOmnibarHistory(false);
        settings.search()->setOmnibarHistory(false);
        QCOMPARE(history.count(), 1);
        settings.search()->setOmnibarBookmarks(false);
        settings.search()->setOmnibarBookmarks(true);
        QCOMPARE(bookmarks.count(), 2);
        settings.search()->setOmnibarDownloads(false);
        QCOMPARE(downloads.count(), 1);
        QCOMPARE(tabs.count(), 1);
        QCOMPARE(history.count(), 1);
    }
    {
        Sections again(path);
        QVERIFY(!again.search()->omnibarTabs());
        QVERIFY(again.search()->omnibarBookmarks());
        QVERIFY(!again.search()->omnibarHistory());
        QVERIFY(!again.search()->omnibarDownloads());
    }
    QSettings raw(path, QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("omnibarTabs")).toBool(), false);
    QCOMPARE(raw.value(QStringLiteral("omnibarBookmarks")).toBool(), true);
    QCOMPARE(raw.value(QStringLiteral("omnibarHistory")).toBool(), false);
    QCOMPARE(raw.value(QStringLiteral("omnibarDownloads")).toBool(), false);
}

void tst_settings::historySwitches()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Sections settings(path);
        QVERIFY(settings.privacy()->rememberHistory());
        QVERIFY(!settings.privacy()->clearHistoryOnClose());
        QSignalSpy remember(settings.privacy(), &PrivacySettings::rememberHistoryChanged);
        QSignalSpy clear(settings.privacy(), &PrivacySettings::clearHistoryOnCloseChanged);

        settings.privacy()->setRememberHistory(true);
        settings.privacy()->setClearHistoryOnClose(false);
        QCOMPARE(remember.count(), 0);
        QCOMPARE(clear.count(), 0);
        settings.privacy()->setRememberHistory(false);
        settings.privacy()->setRememberHistory(false);
        settings.privacy()->setClearHistoryOnClose(true);
        settings.privacy()->setClearHistoryOnClose(true);
        QCOMPARE(remember.count(), 1);
        QCOMPARE(clear.count(), 1);
    }
    Sections again(path);
    QVERIFY(!again.privacy()->rememberHistory());
    QVERIFY(again.privacy()->clearHistoryOnClose());
    QSettings raw(path, QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("rememberHistory")).toBool(), false);
    QCOMPARE(raw.value(QStringLiteral("clearHistoryOnClose")).toBool(), true);
}

void tst_settings::blockNotificationRequests()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Sections settings(path);
        QVERIFY(!settings.privacy()->blockNotificationRequests());
        QSignalSpy spy(settings.privacy(), &PrivacySettings::blockNotificationRequestsChanged);
        settings.privacy()->setBlockNotificationRequests(false);
        QCOMPARE(spy.count(), 0);
        settings.privacy()->setBlockNotificationRequests(true);
        settings.privacy()->setBlockNotificationRequests(true);
        QCOMPARE(spy.count(), 1);
    }
    Sections again(path);
    QVERIFY(again.privacy()->blockNotificationRequests());
    QSettings raw(path, QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("blockNotificationRequests")).toBool(), true);
}

// 1.75 x screen pixel ratio, steps of 0.5.
void tst_settings::pageZoom()
{
    QCOMPARE(Settings::pageZoom(1.0), 2.0);
    QCOMPARE(Settings::pageZoom(1.5), 2.5);
    QCOMPARE(Settings::pageZoom(2.0), 3.5);
    QCOMPARE(Settings::pageZoom(2.25), 4.0);
}

void tst_settings::quickAction()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Sections settings(path);
    QSignalSpy spy(settings.cover(), &CoverSettings::quickActionChanged);

    QCOMPARE(settings.cover()->quickAction(), int(CoverSettings::QuickActionSearch));
    // Stored as numbers: values must not change.
    QCOMPARE(int(CoverSettings::QuickActionNone), 0);
    QCOMPARE(int(CoverSettings::QuickActionSearch), 1);
    QCOMPARE(int(CoverSettings::QuickActionBookmarks), 2);
    QCOMPARE(int(CoverSettings::QuickActionBookmark), 3);
    QCOMPARE(int(CoverSettings::QuickActionDownloads), 4);
    QCOMPARE(int(CoverSettings::QuickActionHistory), 5);

    settings.cover()->setQuickAction(CoverSettings::QuickActionHistory);
    settings.cover()->setQuickAction(CoverSettings::QuickActionHistory);
    QCOMPARE(settings.cover()->quickAction(), int(CoverSettings::QuickActionHistory));
    QCOMPARE(spy.count(), 1);
    settings.cover()->setQuickAction(CoverSettings::QuickActionHistory + 1);
    settings.cover()->setQuickAction(-1);
    QCOMPARE(settings.cover()->quickAction(), int(CoverSettings::QuickActionHistory));
    QCOMPARE(spy.count(), 1);
    settings.cover()->setQuickAction(CoverSettings::QuickActionNone);
    QCOMPARE(spy.count(), 2);
    {
        Sections again(path);
        QCOMPARE(again.cover()->quickAction(), int(CoverSettings::QuickActionNone));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        QCOMPARE(raw.value(QStringLiteral("quickAction")).toInt(), 0);
        raw.setValue(QStringLiteral("quickAction"), 12);
    }
    {
        Sections again(path);
        QCOMPARE(again.cover()->quickAction(), int(CoverSettings::QuickActionSearch));
    }
}

void tst_settings::quickActionBookmark()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    {
        Sections settings(path);
        QSignalSpy spy(settings.cover(), &CoverSettings::quickActionBookmarkChanged);
        QCOMPARE(settings.cover()->quickActionBookmark(), 0);
        QVERIFY(settings.cover()->quickActionBookmarkUrl().isEmpty());
        QVERIFY(settings.cover()->quickActionBookmarkTitle().isEmpty());

        settings.cover()->setQuickActionBookmark(4, QStringLiteral("https://a.example/"),
                                                 QStringLiteral("A"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.cover()->quickActionBookmark(), 4);
        QCOMPARE(settings.cover()->quickActionBookmarkUrl(), QStringLiteral("https://a.example/"));
        QCOMPARE(settings.cover()->quickActionBookmarkTitle(), QStringLiteral("A"));
        settings.cover()->setQuickActionBookmark(4, QStringLiteral("https://a.example/"),
                                                 QStringLiteral("A"));
        QCOMPARE(spy.count(), 1);
        settings.cover()->setQuickActionBookmark(-2, QStringLiteral("https://b.example/"),
                                                 QStringLiteral("B"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.cover()->quickActionBookmark(), 4);

        settings.cover()->setQuickActionBookmark(9, QStringLiteral("https://a.example/"),
                                                 QStringLiteral("A"));
        QCOMPARE(spy.count(), 2);
        settings.cover()->setQuickActionBookmark(9, QStringLiteral("https://a.example/"),
                                                 QStringLiteral("Alpha"));
        QCOMPARE(spy.count(), 3);
        QCOMPARE(settings.cover()->quickAction(), int(CoverSettings::QuickActionSearch));
    }
    {
        Sections again(path);
        QCOMPARE(again.cover()->quickActionBookmark(), 9);
        QCOMPARE(again.cover()->quickActionBookmarkUrl(), QStringLiteral("https://a.example/"));
        QCOMPARE(again.cover()->quickActionBookmarkTitle(), QStringLiteral("Alpha"));

        QSignalSpy spy(again.cover(), &CoverSettings::quickActionBookmarkChanged);
        again.cover()->setQuickActionBookmark(0, QStringLiteral("https://a.example/"),
                                              QStringLiteral("Alpha"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(again.cover()->quickActionBookmark(), 0);
        QVERIFY(again.cover()->quickActionBookmarkUrl().isEmpty());
        QVERIFY(again.cover()->quickActionBookmarkTitle().isEmpty());
        again.cover()->setQuickActionBookmark(0, QString(), QString());
        QCOMPARE(spy.count(), 1);
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("quickActionBookmark"), -5);
    }
    Sections again(path);
    QCOMPARE(again.cover()->quickActionBookmark(), 0);
}

void tst_settings::quickActionIcon()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).absoluteFilePath(QStringLiteral("salama.conf"));
    Sections settings(path);
    QSignalSpy spy(settings.cover(), &CoverSettings::quickActionIconChanged);

    QCOMPARE(settings.cover()->quickActionIcons(),
             (QStringList{QStringLiteral("globe"), QStringLiteral("heart"), QStringLiteral("home"),
                          QStringLiteral("work"), QStringLiteral("news"), QStringLiteral("music"),
                          QStringLiteral("shop"), QStringLiteral("star")}));
    QCOMPARE(settings.cover()->quickActionIcon(), QStringLiteral("globe"));

    settings.cover()->setQuickActionIcon(QStringLiteral("music"));
    settings.cover()->setQuickActionIcon(QStringLiteral("music"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.cover()->quickActionIcon(), QStringLiteral("music"));
    settings.cover()->setQuickActionIcon(QStringLiteral("rocket"));
    settings.cover()->setQuickActionIcon(QString());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.cover()->quickActionIcon(), QStringLiteral("music"));
    {
        Sections again(path);
        QCOMPARE(again.cover()->quickActionIcon(), QStringLiteral("music"));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("quickActionIcon"), QStringLiteral("Music"));
    }
    Sections again(path);
    QCOMPARE(again.cover()->quickActionIcon(), QStringLiteral("globe"));
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
    // Half rounds up, like Math.round().
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

// Every quick action glyph (per kind + bookmark-assignable) at coverIconPath(), every size,
// both inks, black != white: icons/render.sh derives black by rewriting white's one colour
// spelling; other white spelling would yield white twice.
void tst_settings::coverIconPathNamesEveryGlyph()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    const QDir source(QStringLiteral(SALAMA_SOURCE_DIR));
    const QStringList names = QStringList{QStringLiteral("search"), QStringLiteral("bookmarks"),
                                          QStringLiteral("downloads"), QStringLiteral("history")} +
                              settings.cover()->quickActionIcons();
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

void tst_settings::startPage()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QVERIFY(!settings.startPage()->blank());
        QVERIFY(settings.startPage()->topSites());
        QVERIFY(settings.startPage()->bookmarks());
        QVERIFY(settings.startPage()->recent());

        QSignalSpy spy(settings.startPage(), &StartPageSettings::changed);
        settings.startPage()->setTopSites(true);
        settings.startPage()->setBlank(false);
        QCOMPARE(spy.count(), 0);
        settings.startPage()->setBlank(true);
        settings.startPage()->setBlank(true);
        QCOMPARE(spy.count(), 1);
        settings.startPage()->setTopSites(false);
        settings.startPage()->setBookmarks(false);
        settings.startPage()->setRecent(false);
        QCOMPARE(spy.count(), 4);
        settings.startPage()->setRecent(true);
        QCOMPARE(spy.count(), 5);
    }
    Sections reloaded(path);
    QVERIFY(reloaded.startPage()->blank());
    QVERIFY(!reloaded.startPage()->topSites());
    QVERIFY(!reloaded.startPage()->bookmarks());
    QVERIFY(reloaded.startPage()->recent());
}

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
        QVERIFY(!settings.general()->cutoutGuard());
    }
    QSettings file(path, QSettings::IniFormat);
    QVERIFY(!file.contains(QStringLiteral("homePage")));
    QVERIFY(file.contains(QStringLiteral("notchGuard")));
}

void tst_settings::tutorialShown()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QVERIFY(!settings.general()->tutorialShown());
        QSignalSpy spy(settings.general(), &Settings::tutorialShownChanged);
        settings.general()->setTutorialShown(false);
        QCOMPARE(spy.count(), 0);
        settings.general()->setTutorialShown(true);
        settings.general()->setTutorialShown(true);
        QCOMPARE(spy.count(), 1);
    }
    Sections reloaded(path);
    QVERIFY(reloaded.general()->tutorialShown());
}

void tst_settings::websiteColors()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QCOMPARE(settings.general()->websiteColors(), int(Settings::WebsiteColorsAutomatic));
        QSignalSpy spy(settings.general(), &Settings::websiteColorsChanged);
        settings.general()->setWebsiteColors(Settings::WebsiteColorsAutomatic);
        settings.general()->setWebsiteColors(Settings::WebsiteColorsDark + 1);
        settings.general()->setWebsiteColors(-1);
        QCOMPARE(spy.count(), 0);
        settings.general()->setWebsiteColors(Settings::WebsiteColorsDark);
        settings.general()->setWebsiteColors(Settings::WebsiteColorsDark);
        QCOMPARE(spy.count(), 1);
    }
    {
        Sections reloaded(path);
        QCOMPARE(reloaded.general()->websiteColors(), int(Settings::WebsiteColorsDark));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("websiteColors"), 7);
    }
    Sections again(path);
    QCOMPARE(again.general()->websiteColors(), int(Settings::WebsiteColorsAutomatic));
}

void tst_settings::isSearchUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("search");

    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    for (int i = 0; i < settings.searchEngines()->engineKeys().count(); ++i) {
        settings.search()->setEngineIndex(i);
        const QString results = settings.search()->searchUrl(QStringLiteral("sailfish os"));
        QTest::newRow(qPrintable(settings.searchEngines()->engineKeys().at(i))) << results << true;
    }
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
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    QCOMPARE(settings.searchEngines()->isSearchUrl(url), search);
}

// sailfish-browser's three notch guard modes stored as chosen; out-of-range refused;
// cutout avoidance follows mode.
void tst_settings::notchGuard()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy spy(settings.general(), &Settings::notchGuardChanged);
        settings.general()->setNotchGuard(Settings::NotchGuardForced);
        QCOMPARE(spy.count(), 1);
        QVERIFY(settings.general()->cutoutGuard());
        settings.general()->setNotchGuard(3);
        settings.general()->setNotchGuard(-1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.general()->notchGuard(), int(Settings::NotchGuardForced));
    }
    Sections reloaded(path);
    QCOMPARE(reloaded.general()->notchGuard(), int(Settings::NotchGuardForced));
    {
        QSettings file(path, QSettings::IniFormat);
        file.setValue(QStringLiteral("notchGuard"), 7);
    }
    Sections edited(path);
    QCOMPARE(edited.general()->notchGuard(), int(Settings::NotchGuardAutomatic));
}

void tst_settings::notchGuardOfAnEarlierRelease_data()
{
    QTest::addColumn<QVariant>("cutoutGuard");
    QTest::addColumn<QVariant>("notchGuard");
    QTest::addColumn<int>("expected");

    // Old switch: on = every page below cutout, off = none.
    QTest::newRow("switched on") << QVariant(true) << QVariant() << int(Settings::NotchGuardForced);
    QTest::newRow("switched off") << QVariant(false) << QVariant()
                                  << int(Settings::NotchGuardDisabled);
    QTest::newRow("never switched")
        << QVariant() << QVariant() << int(Settings::NotchGuardAutomatic);
    QTest::newRow("both") << QVariant(false) << QVariant(int(Settings::NotchGuardAutomatic))
                          << int(Settings::NotchGuardAutomatic);
}

void tst_settings::notchGuardOfAnEarlierRelease()
{
    QFETCH(QVariant, cutoutGuard);
    QFETCH(QVariant, notchGuard);
    QFETCH(int, expected);

    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        QSettings old(path, QSettings::IniFormat);
        if (cutoutGuard.isValid()) {
            old.setValue(QStringLiteral("cutoutGuard"), cutoutGuard);
        }
        if (notchGuard.isValid()) {
            old.setValue(QStringLiteral("notchGuard"), notchGuard);
        }
    }
    {
        Sections settings(path);
        QCOMPARE(settings.general()->notchGuard(), expected);
    }
    QSettings file(path, QSettings::IniFormat);
    QVERIFY(!file.contains(QStringLiteral("cutoutGuard")));
    Sections reloaded(path);
    QCOMPARE(reloaded.general()->notchGuard(), expected);
}

void tst_settings::fixedToolbar()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy spy(settings.general(), &Settings::fixedToolbarChanged);
        settings.general()->setFixedToolbar(true);
        settings.general()->setFixedToolbar(true);
        QCOMPARE(spy.count(), 1);
    }
    Sections reloaded(path);
    QVERIFY(reloaded.general()->fixedToolbar());
}

void tst_settings::linkPreview()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy spy(settings.general(), &Settings::linkPreviewChanged);
        settings.general()->setLinkPreview(true);
        QCOMPARE(spy.count(), 0);
        settings.general()->setLinkPreview(false);
        settings.general()->setLinkPreview(false);
        QCOMPARE(spy.count(), 1);
    }
    {
        Sections reloaded(path);
        QVERIFY(!reloaded.general()->linkPreview());
        reloaded.general()->setLinkPreview(true);
    }
    Sections again(path);
    QVERIFY(again.general()->linkPreview());
}

void tst_settings::contentSwitches()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy gpcSpy(settings.privacy(), &PrivacySettings::globalPrivacyControlChanged);
        QSignalSpy scriptSpy(settings.privacy(), &PrivacySettings::javascriptChanged);
        settings.privacy()->setGlobalPrivacyControl(true);
        settings.privacy()->setGlobalPrivacyControl(true);
        settings.privacy()->setJavascript(false);
        settings.privacy()->setJavascript(false);
        QCOMPARE(gpcSpy.count(), 1);
        QCOMPARE(scriptSpy.count(), 1);
    }
    Sections reloaded(path);
    QVERIFY(reloaded.privacy()->globalPrivacyControl());
    QVERIFY(!reloaded.privacy()->javascript());
}

// Old Do Not Track file: GPC starts as DNT was, old key removed; existing GPC kept.
void tst_settings::globalPrivacyControlTakesDoNotTracksPlace_data()
{
    QTest::addColumn<QVariant>("doNotTrack");
    QTest::addColumn<QVariant>("globalPrivacyControl");
    QTest::addColumn<bool>("expected");
    QTest::newRow("neither") << QVariant() << QVariant() << false;
    QTest::newRow("do not track on") << QVariant(true) << QVariant() << true;
    QTest::newRow("do not track off") << QVariant(false) << QVariant() << false;
    QTest::newRow("both, gpc wins") << QVariant(true) << QVariant(false) << false;
    QTest::newRow("gpc alone") << QVariant() << QVariant(true) << true;
}

void tst_settings::globalPrivacyControlTakesDoNotTracksPlace()
{
    QFETCH(QVariant, doNotTrack);
    QFETCH(QVariant, globalPrivacyControl);
    QFETCH(bool, expected);
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        QSettings file(path, QSettings::IniFormat);
        if (doNotTrack.isValid()) {
            file.setValue(QStringLiteral("doNotTrack"), doNotTrack);
        }
        if (globalPrivacyControl.isValid()) {
            file.setValue(QStringLiteral("globalPrivacyControl"), globalPrivacyControl);
        }
    }
    {
        Sections settings(path);
        QCOMPARE(settings.privacy()->globalPrivacyControl(), expected);
    }
    QSettings file(path, QSettings::IniFormat);
    QVERIFY(!file.contains(QStringLiteral("doNotTrack")));
    Sections reloaded(path);
    QCOMPARE(reloaded.privacy()->globalPrivacyControl(), expected);
}

void tst_settings::httpsOnly()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy spy(settings.privacy(), &PrivacySettings::httpsOnlyChanged);
        settings.privacy()->setHttpsOnly(true);
        settings.privacy()->setHttpsOnly(true);
        QCOMPARE(spy.count(), 1);
    }
    Sections reloaded(path);
    QVERIFY(reloaded.privacy()->httpsOnly());
}

void tst_settings::dohProtection()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy spy(settings.doh(), &DohSettings::protectionChanged);
        settings.doh()->setProtection(DohSettings::ProtectionMax);
        settings.doh()->setProtection(DohSettings::ProtectionMax);
        settings.doh()->setProtection(3);
        settings.doh()->setProtection(-1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.doh()->protection(), int(DohSettings::ProtectionMax));
    }
    {
        Sections reloaded(path);
        QCOMPARE(reloaded.doh()->protection(), int(DohSettings::ProtectionMax));
        reloaded.doh()->setProtection(DohSettings::ProtectionIncreased);
    }
    {
        Sections reloaded(path);
        QCOMPARE(reloaded.doh()->protection(), int(DohSettings::ProtectionIncreased));
    }
    {
        QSettings file(path, QSettings::IniFormat);
        file.setValue(QStringLiteral("dohProtection"), 7);
    }
    Sections edited(path);
    QCOMPARE(edited.doh()->protection(), int(DohSettings::ProtectionOff));
}

void tst_settings::dohProvider()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    const QString nextDns = QStringLiteral("https://firefox.dns.nextdns.io/");
    const QString own = QStringLiteral("https://dns.example.org/dns-query");
    {
        Sections settings(path);
        const QVariantList providers = settings.doh()->providers();
        QCOMPARE(providers.count(), 2);
        QCOMPARE(providers.at(0).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("Cloudflare"));
        QCOMPARE(providers.at(0).toMap().value(QStringLiteral("url")).toString(),
                 QStringLiteral("https://mozilla.cloudflare-dns.com/dns-query"));
        QCOMPARE(DohSettings::defaultProvider(),
                 providers.at(0).toMap().value(QStringLiteral("url")).toString());
        QCOMPARE(providers.at(1).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("NextDNS"));
        QCOMPARE(providers.at(1).toMap().value(QStringLiteral("url")).toString(), nextDns);
        QVERIFY(!settings.doh()->customProvider());

        QSignalSpy spy(settings.doh(), &DohSettings::providerChanged);
        settings.doh()->setProvider(nextDns);
        settings.doh()->setProvider(nextDns);
        QCOMPARE(spy.count(), 1);
        QVERIFY(!settings.doh()->customProvider());
        settings.doh()->setProvider(QStringLiteral("http://dns.example.org/"));
        settings.doh()->setProvider(QStringLiteral("not an address"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.doh()->provider(), nextDns);
        settings.doh()->setProvider(QStringLiteral("  ") + own + QStringLiteral(" "));
        QCOMPARE(spy.count(), 2);
        QCOMPARE(settings.doh()->provider(), own);
        QVERIFY(settings.doh()->customProvider());
    }
    {
        Sections reloaded(path);
        QCOMPARE(reloaded.doh()->provider(), own);
    }
    {
        QSettings file(path, QSettings::IniFormat);
        file.setValue(QStringLiteral("dohProvider"), QStringLiteral("ftp://nope"));
    }
    Sections edited(path);
    QCOMPARE(edited.doh()->provider(), DohSettings::defaultProvider());
    QVERIFY(!edited.doh()->customProvider());
}

void tst_settings::dohProviderProblem_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<int>("problem");
    QTest::newRow("valid") << QStringLiteral("https://dns.example.org/dns-query")
                           << int(DohSettings::ProviderValid);
    QTest::newRow("bare host") << QStringLiteral("https://dns.example.org")
                               << int(DohSettings::ProviderValid);
    QTest::newRow("http") << QStringLiteral("http://dns.example.org/")
                          << int(DohSettings::ProviderNotHttps);
    QTest::newRow("no scheme") << QStringLiteral("dns.example.org")
                               << int(DohSettings::ProviderNotHttps);
    QTest::newRow("one slash") << QStringLiteral("https:/dns.example.org")
                               << int(DohSettings::ProviderNotHttps);
    QTest::newRow("upper case scheme")
        << QStringLiteral("HTTPS://dns.example.org") << int(DohSettings::ProviderNotHttps);
    QTest::newRow("empty") << QString() << int(DohSettings::ProviderNotHttps);
    QTest::newRow("no host") << QStringLiteral("https://") << int(DohSettings::ProviderInvalid);
    QTest::newRow("no host, a path")
        << QStringLiteral("https:///dns-query") << int(DohSettings::ProviderInvalid);
    QTest::newRow("space in host")
        << QStringLiteral("https://dns example.org/") << int(DohSettings::ProviderInvalid);
}

void tst_settings::dohProviderProblem()
{
    QFETCH(QString, url);
    QFETCH(int, problem);
    QCOMPARE(DohSettings::providerProblem(url), problem);
}

void tst_settings::dohDomainOf_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("domain");
    QTest::newRow("domain") << QStringLiteral("example.com") << QStringLiteral("example.com");
    QTest::newRow("upper case") << QStringLiteral("Example.COM") << QStringLiteral("example.com");
    QTest::newRow("scheme and path") << QStringLiteral("https://intranet.example.com/a?b")
                                     << QStringLiteral("intranet.example.com");
    QTest::newRow("other scheme") << QStringLiteral("http://router.local:8080")
                                  << QStringLiteral("router.local");
    QTest::newRow("spaces around")
        << QStringLiteral("  example.com ") << QStringLiteral("example.com");
    QTest::newRow("space inside") << QStringLiteral("exam ple.com") << QString();
    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("scheme alone") << QStringLiteral("https://") << QString();
    QTest::newRow("path alone") << QStringLiteral("/path") << QString();
}

void tst_settings::dohDomainOf()
{
    QFETCH(QString, text);
    QFETCH(QString, domain);
    QCOMPARE(DohSettings::domainOf(text), domain);
}

void tst_settings::dohExceptions()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QSignalSpy spy(settings.doh(), &DohSettings::exceptionsChanged);
        QVERIFY(settings.doh()->addException(QStringLiteral("https://Intranet.example.com/")));
        QVERIFY(settings.doh()->addException(QStringLiteral("router.local")));
        QVERIFY(!settings.doh()->addException(QStringLiteral("intranet.example.com")));
        QVERIFY(!settings.doh()->addException(QStringLiteral("not a domain")));
        QCOMPARE(spy.count(), 2);
        QCOMPARE(settings.doh()->exceptions(), (QStringList{QStringLiteral("intranet.example.com"),
                                                            QStringLiteral("router.local")}));
        settings.doh()->removeException(QStringLiteral("elsewhere.example"));
        QCOMPARE(spy.count(), 2);
    }
    {
        Sections reloaded(path);
        QCOMPARE(reloaded.doh()->exceptions().count(), 2);
        QSignalSpy spy(reloaded.doh(), &DohSettings::exceptionsChanged);
        reloaded.doh()->removeException(QStringLiteral("intranet.example.com"));
        QCOMPARE(reloaded.doh()->exceptions(), QStringList{QStringLiteral("router.local")});
        reloaded.doh()->addException(QStringLiteral("a.example"));
        reloaded.doh()->removeAllExceptions();
        reloaded.doh()->removeAllExceptions();
        QCOMPARE(spy.count(), 3);
        QVERIFY(reloaded.doh()->exceptions().isEmpty());
    }
    Sections again(path);
    QVERIFY(again.doh()->exceptions().isEmpty());
}

QTEST_GUILESS_MAIN(tst_settings)
#include "tst_settings.moc"
