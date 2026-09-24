// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Core.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::BookmarkModel;
using Salama::Core;
using Salama::DownloadModel;
using Salama::HistoryModel;
using Salama::Settings;

class tst_core : public QObject
{
    Q_OBJECT

private slots:
    void wiresTabsToHistory();
    void wiresFaviconsAndActiveUrlToBookmarks();
    void restoresState();
    void wiresPlaybackToPageMedia();
};

void tst_core::wiresTabsToHistory()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"));
    QVERIFY(core.storage().isOpen());
    QVERIFY(core.engineMessages() != nullptr);
    QVERIFY(core.settings() != nullptr);
    QVERIFY(core.tabSearch() != nullptr);
    QCOMPARE(core.tabSearch()->count(), 0);
    QVERIFY(core.downloads() != nullptr);
    QCOMPARE(core.downloads()->count(), 0);

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
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"));
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
        Core core(dir.path(), config);
        core.tabs()->newTab(QStringLiteral("https://a.example/"));
        core.settings()->setDesktopMode(true);
        core.downloads()->observe(
            core.downloads()->topic(),
            QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-start")},
                        {QStringLiteral("id"), 1.0},
                        {QStringLiteral("displayName"), QStringLiteral("a.pdf")}});
    }
    Core core(dir.path(), config);
    QCOMPARE(core.tabs()->count(), 1);
    // The downloads are kept in the same database; one that was running did not finish.
    QCOMPARE(core.downloads()->count(), 1);
    QCOMPARE(
        core.downloads()->data(core.downloads()->index(0, 0), DownloadModel::StatusRole).toInt(),
        static_cast<int>(DownloadModel::Failed));
    QCOMPARE(core.bookmarks()->activeUrl(), QStringLiteral("https://a.example/"));
    QVERIFY(core.settings()->desktopMode());
    // The tab model takes its live-page limit from Settings, and follows it.
    QCOMPARE(core.tabs()->liveTabLimit(), Settings::defaultLiveTabLimit());
    core.settings()->setLiveTabLimitIndex(0);
    QCOMPARE(core.tabs()->liveTabLimit(), core.settings()->liveTabLimit());
    QCOMPARE(core.tabs()->liveTabLimit(), 3);
}

// The engine's word that something plays, which PageActivity hears, has every loaded
// page asked what it plays (docs/DECISIONS/0025-media-controls.md).
void tst_core::wiresPlaybackToPageMedia()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"));
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
}

QTEST_GUILESS_MAIN(tst_core)
#include "tst_core.moc"
