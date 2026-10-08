// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Core.h"
#include "tabs/ClosedTabModel.h"

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::BookmarkModel;
using Salama::Core;
using Salama::DownloadModel;
using Salama::HistoryModel;

class tst_core : public QObject
{
    Q_OBJECT

private slots:
    void wiresTabsToHistory();
    void wiresFaviconsAndActiveUrlToBookmarks();
    void restoresState();
    void wiresPlaybackToPageMedia();
    void omnibarSearchesTheModels();
    void historyNotRemembered();
    void clearsOnClose();
    void wiresHistoryAndBookmarksToTheStartPage();
    void wiresAddedEnginesToTheStartPage();
};

void tst_core::wiresTabsToHistory()
{
    QTemporaryDir dir;
    const QString downloads = dir.path() + QStringLiteral("/Downloads/Salama");
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), downloads);
    QVERIFY(core.storage().isOpen());
    QVERIFY(core.engineMessages() != nullptr);
    QVERIFY(core.settings() != nullptr);
    QVERIFY(core.tabSearch() != nullptr);
    QCOMPARE(core.tabSearch()->count(), 0);
    QVERIFY(core.downloads() != nullptr);
    QCOMPARE(core.downloads()->count(), 0);
    QCOMPARE(core.downloads()->directory(), downloads);
    QVERIFY(QFileInfo(downloads).isDir());

    const int id = core.tabs()->newTab(QStringLiteral("https://a.example/"));
    QCOMPARE(core.history()->count(), 0);
    core.tabs()->updateUrl(id, QStringLiteral("https://a.example/"));
    QCOMPARE(core.history()->count(), 1);
    QCOMPARE(core.tabSearch()->count(), 1);
    core.tabs()->updateTitle(id, QStringLiteral("Alpha"));
    QCOMPARE(core.history()
                 ->data(core.history()->index(0, 0), roleId(HistoryModel::Role::Title))
                 .toString(),
             QStringLiteral("Alpha"));
}

void tst_core::wiresFaviconsAndActiveUrlToBookmarks()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    QVERIFY(core.bookmarks()->activeUrl().isEmpty());

    const int id = core.tabs()->newTab(QStringLiteral("https://a.example/"));
    QCOMPARE(core.bookmarks()->activeUrl(), QStringLiteral("https://a.example/"));
    core.bookmarks()->add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    QVERIFY(core.bookmarks()->activeUrlBookmarked());

    core.tabs()->updateFavicon(id, QStringLiteral("https://a.example/icon.png"));
    QCOMPARE(core.bookmarks()
                 ->data(core.bookmarks()->index(0, 0), roleId(BookmarkModel::Role::Favicon))
                 .toString(),
             QStringLiteral("https://a.example/icon.png"));

    core.tabs()->updateUrl(id, QStringLiteral("https://a.example/other"));
    QVERIFY(!core.bookmarks()->activeUrlBookmarked());
}

void tst_core::restoresState()
{
    QTemporaryDir dir;
    const QString config = dir.path() + QStringLiteral("/salama.conf");
    {
        Core core(dir.path(), config, dir.path());
        core.tabs()->newTab(QStringLiteral("https://a.example/"));
        core.downloads()->observe(
            core.downloads()->topic(),
            QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-start")},
                        {QStringLiteral("id"), 1.0},
                        {QStringLiteral("displayName"), QStringLiteral("a.pdf")}});
    }
    Core core(dir.path(), config, dir.path());
    QCOMPARE(core.tabs()->count(), 1);
    QCOMPARE(core.downloads()->count(), 1);
    QCOMPARE(core.downloads()
                 ->data(core.downloads()->index(0, 0), roleId(DownloadModel::Role::Status))
                 .toInt(),
             static_cast<int>(DownloadModel::Failed));
    QCOMPARE(core.bookmarks()->activeUrl(), QStringLiteral("https://a.example/"));
    // Five live pages, as Jolla browser.
    QCOMPARE(core.tabs()->liveTabLimit(), 5);
}

// Engine playback notice (via PageActivity) makes every loaded page report what plays.
void tst_core::wiresPlaybackToPageMedia()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    QVERIFY(core.pageMedia() != nullptr);
    core.tabs()->newTab(QStringLiteral("https://a.example/"));
    QSignalSpy requested(core.pageMedia(), &Salama::PageMedia::requested);
    QVERIFY(requested.wait(core.pageMedia()->queryDelay() * 10));
    requested.clear();
    core.pageActivity()->observe(QStringLiteral("media-decoder-info"),
                                 QVariantMap{{QStringLiteral("owner"), QStringLiteral("0x1")},
                                             {QStringLiteral("state"), QStringLiteral("play")}});
    QVERIFY(requested.wait(core.pageMedia()->queryDelay() * 10));
    QCOMPARE(requested.first().at(0).toInt(), 0);
    QCOMPARE(requested.first().at(1).toInt(), static_cast<int>(Salama::PageMedia::Command::Query));

    const int id = core.tabs()->activeTabId();
    core.pageActivity()->setBackground(true);
    QVERIFY(core.pageMedia()
                ->script(id, Salama::PageMedia::Command::Query)
                .contains(QLatin1String("concealed = true;")));
    core.pageActivity()->setBackground(false);
    QVERIFY(core.pageMedia()
                ->script(id, Salama::PageMedia::Command::Query)
                .contains(QLatin1String("concealed = false;")));
}

void tst_core::omnibarSearchesTheModels()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    Salama::OmnibarModel *omnibar = core.omnibar();
    QVERIFY(omnibar != nullptr);
    const int id = core.tabs()->newTab(QStringLiteral("https://core.example/"));
    core.tabs()->updateUrl(id, QStringLiteral("https://core.example/"));
    core.tabs()->newTab(QStringLiteral("https://front.example/"));
    core.bookmarks()->add(QStringLiteral("https://core.example/bookmark"), QStringLiteral("B"));

    omnibar->setQuery(QStringLiteral("core"));
    QCOMPARE(core.history()->count(), 1);
    QCOMPARE(omnibar->count(), 2);
    const auto kinds = [omnibar]() {
        QStringList kinds;
        for (int row = 0; row < omnibar->rowCount(); ++row) {
            kinds.append(
                omnibar
                    ->data(omnibar->index(row, 0), Salama::roleId(Salama::OmnibarModel::Role::Kind))
                    .toString());
        }
        kinds.sort();
        return kinds;
    };
    QCOMPARE(kinds(), (QStringList{QStringLiteral("bookmark"), QStringLiteral("tab")}));

    core.searchSettings()->setOmnibarBookmarks(false);
    QTRY_COMPARE(kinds(), QStringList{QStringLiteral("tab")});
}

void tst_core::historyNotRemembered()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    const auto load = [&core](const QString &url) {
        core.tabs()->updateUrl(core.tabs()->newTab(url), url);
    };
    load(QStringLiteral("https://kept.example/"));
    QCOMPARE(core.history()->count(), 1);
    core.privacySettings()->setRememberHistory(false);
    load(QStringLiteral("https://unkept.example/"));
    QCOMPARE(core.history()->count(), 1);
    core.privacySettings()->setRememberHistory(true);
    load(QStringLiteral("https://kept-again.example/"));
    QCOMPARE(core.history()->count(), 2);
}

// When set: history, downloads, closed tabs cleared on close, and on start after unclean
// stop. Tabs and bookmarks stay.
void tst_core::clearsOnClose()
{
    QTemporaryDir dir;
    const QString config = dir.path() + QStringLiteral("/salama.conf");
    {
        Core core(dir.path(), config, dir.path());
        const auto load = [&core](const QString &url) {
            const int id = core.tabs()->newTab(url);
            core.tabs()->updateUrl(id, url);
            return id;
        };
        const int closed = load(QStringLiteral("https://closed.example/"));
        load(QStringLiteral("https://open.example/"));
        core.tabs()->closeTabById(closed);
        core.bookmarks()->add(QStringLiteral("https://mark.example/"), QStringLiteral("Mark"));
        core.downloads()->observe(
            core.downloads()->topic(),
            QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-start")},
                        {QStringLiteral("id"), 1.0},
                        {QStringLiteral("displayName"), QStringLiteral("a.pdf")},
                        {QStringLiteral("sourceUrl"), QStringLiteral("https://f.example/a.pdf")},
                        {QStringLiteral("targetPath"), QString()}});
        QCOMPARE(core.history()->count(), 2);
        QCOMPARE(core.downloads()->count(), 1);
        QCOMPARE(core.tabs()->closedTabs()->count(), 1);

        QVERIFY(!core.privacySettings()->clearHistoryOnClose());
        core.clearOnClose();
        QCOMPARE(core.history()->count(), 2);
        QCOMPARE(core.downloads()->count(), 1);

        core.privacySettings()->setClearHistoryOnClose(true);
        core.clearOnClose();
        QCOMPARE(core.history()->count(), 0);
        QCOMPARE(core.downloads()->count(), 0);
        QCOMPARE(core.tabs()->closedTabs()->count(), 0);
        QCOMPARE(core.tabs()->count(), 1);
        QCOMPARE(core.bookmarks()->count(), 1);

        load(QStringLiteral("https://later.example/"));
        QCOMPARE(core.history()->count(), 1);
    }
    Core again(dir.path(), config, dir.path());
    QCOMPARE(again.history()->count(), 0);
    QCOMPARE(again.bookmarks()->count(), 1);
}

void tst_core::wiresHistoryAndBookmarksToTheStartPage()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    Salama::StartPage *start = core.startPage();
    QVERIFY(start != nullptr);
    QCOMPARE(start->topSites()->count(), 0);

    const int id = core.tabs()->newTab(QString());
    core.tabs()->updateUrl(id, QStringLiteral("https://a.example/"));
    QCOMPARE(start->topSites()->count(), 1);
    QCOMPARE(start->recentPages()->count(), 1);
    core.tabs()->updateTitle(id, QStringLiteral("Alpha"));
    QCOMPARE(start->recentPages()->sites().first().title, QStringLiteral("Alpha"));
    core.tabs()->updateFavicon(id, QStringLiteral("https://a.example/icon.png"));
    QCOMPARE(start->topSites()->sites().first().favicon,
             QStringLiteral("https://a.example/icon.png"));

    core.bookmarks()->add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    QCOMPARE(start->bookmarks()->count(), 1);
    core.bookmarks()->edit(0, QStringLiteral("https://a.example/"), QStringLiteral("Mark"));
    QCOMPARE(start->bookmarks()->sites().first().title, QStringLiteral("Mark"));
    core.bookmarks()->remove(0);
    QCOMPARE(start->bookmarks()->count(), 0);

    core.history()->removeUrl(QStringLiteral("https://a.example/"));
    QCOMPARE(start->topSites()->count(), 0);
    QCOMPARE(start->recentPages()->count(), 0);
}

// Result page of engine added while browsing = search, not visit, from moment of add,
// without needing visit to refresh start page.
void tst_core::wiresAddedEnginesToTheStartPage()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    const int id = core.tabs()->newTab(QString());
    core.tabs()->updateUrl(id, QStringLiteral("https://find.example/results?query=forest"));
    QCOMPARE(core.startPage()->topSites()->count(), 1);

    const QString href = QStringLiteral("https://find.example/opensearch.xml");
    QVERIFY(core.searchEngines()->offerEngine(QStringLiteral("Find"), href, QString()));
    QVERIFY(core.searchEngines()->addFoundEngine(
        href, QStringLiteral("<OpenSearchDescription><ShortName>Find</ShortName>"
                             "<Url type=\"text/html\" template=\"https://find.example/results?"
                             "query={searchTerms}\"/></OpenSearchDescription>")));
    QCOMPARE(core.startPage()->topSites()->count(), 0);

    core.searchEngines()->removeAddedEngines();
    QCOMPARE(core.startPage()->topSites()->count(), 1);
}

QTEST_GUILESS_MAIN(tst_core)
#include "tst_core.moc"
