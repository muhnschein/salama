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
    QCOMPARE(core.history()->data(core.history()->index(0, 0), HistoryModel::TitleRole).toString(),
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
                 ->data(core.bookmarks()->index(0, 0), BookmarkModel::FaviconRole)
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
    // The downloads are kept in the same database; one that was running did not finish.
    QCOMPARE(core.downloads()->count(), 1);
    QCOMPARE(
        core.downloads()->data(core.downloads()->index(0, 0), DownloadModel::StatusRole).toInt(),
        static_cast<int>(DownloadModel::Failed));
    QCOMPARE(core.bookmarks()->activeUrl(), QStringLiteral("https://a.example/"));
    // Five pages stay loaded, as in Jolla's browser.
    QCOMPARE(core.tabs()->liveTabLimit(), 5);
}

// The engine's word that something plays, which PageActivity hears, has every loaded
// page asked what it plays (docs/DECISIONS/0026-media-controls.md).
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
    QCOMPARE(requested.first().at(1).toInt(), static_cast<int>(Salama::PageMedia::Query));

    // Out of sight, the pages hear of it: what plays is hidden from them.
    const int id = core.tabs()->activeTabId();
    core.pageActivity()->setBackground(true);
    QVERIFY(core.pageMedia()
                ->script(id, Salama::PageMedia::Query)
                .contains(QLatin1String("concealed = true;")));
    core.pageActivity()->setBackground(false);
    QVERIFY(core.pageMedia()
                ->script(id, Salama::PageMedia::Query)
                .contains(QLatin1String("concealed = false;")));
}

// The address bar's suggestions are drawn from the core's own models, and follow the
// core's settings (docs/DECISIONS/0027-omnibar.md).
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
    // The tab's visit went to the history, and is listed as the tab it is.
    QCOMPARE(core.history()->count(), 1);
    QCOMPARE(omnibar->count(), 2);
    const auto kinds = [omnibar]() {
        QStringList kinds;
        for (int row = 0; row < omnibar->rowCount(); ++row) {
            kinds.append(
                omnibar->data(omnibar->index(row, 0), Salama::OmnibarModel::KindRole).toString());
        }
        kinds.sort();
        return kinds;
    };
    QCOMPARE(kinds(), (QStringList{QStringLiteral("bookmark"), QStringLiteral("tab")}));

    core.settings()->setOmnibarBookmarks(false);
    QTRY_COMPARE(kinds(), QStringList{QStringLiteral("tab")});
}

// A page visited while the history is not to be kept is not kept; what was kept stays
// (docs/DECISIONS/0030-history-settings.md).
void tst_core::historyNotRemembered()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    const auto load = [&core](const QString &url) {
        core.tabs()->updateUrl(core.tabs()->newTab(url), url);
    };
    load(QStringLiteral("https://kept.example/"));
    QCOMPARE(core.history()->count(), 1);
    core.settings()->setRememberHistory(false);
    load(QStringLiteral("https://unkept.example/"));
    QCOMPARE(core.history()->count(), 1);
    core.settings()->setRememberHistory(true);
    load(QStringLiteral("https://kept-again.example/"));
    QCOMPARE(core.history()->count(), 2);
}

// Set to, the history, the list of downloads and the recently closed tabs go as the
// browser closes -- and as it starts, for a browser stopped before it could. The tabs
// themselves and the bookmarks stay.
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

        // Off unless switched on: closing leaves everything.
        QVERIFY(!core.settings()->clearHistoryOnClose());
        core.clearOnClose();
        QCOMPARE(core.history()->count(), 2);
        QCOMPARE(core.downloads()->count(), 1);

        core.settings()->setClearHistoryOnClose(true);
        core.clearOnClose();
        QCOMPARE(core.history()->count(), 0);
        QCOMPARE(core.downloads()->count(), 0);
        QCOMPARE(core.tabs()->closedTabs()->count(), 0);
        QCOMPARE(core.tabs()->count(), 1);
        QCOMPARE(core.bookmarks()->count(), 1);

        // A page after that, and the browser stopped rather than closed.
        load(QStringLiteral("https://later.example/"));
        QCOMPARE(core.history()->count(), 1);
    }
    Core again(dir.path(), config, dir.path());
    QCOMPARE(again.history()->count(), 0);
    QCOMPARE(again.bookmarks()->count(), 1);
}

// The start page follows the history and the bookmarks as they change, icons included
// (docs/DECISIONS/0032-start-page.md).
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

QTEST_GUILESS_MAIN(tst_core)
#include "tst_core.moc"
