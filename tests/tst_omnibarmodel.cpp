// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "bookmarks/BookmarkModel.h"
#include "downloads/DownloadModel.h"
#include "history/HistoryModel.h"
#include "omnibar/OmnibarModel.h"
#include "settings/Settings.h"
#include "storage/Storage.h"
#include "tabs/TabModel.h"

#include <QDateTime>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

using Salama::BookmarkModel;
using Salama::DownloadModel;
using Salama::HistoryModel;
using Salama::OmnibarModel;
using Salama::Settings;
using Salama::Storage;
using Salama::TabModel;

class tst_omnibarmodel : public QObject
{
    Q_OBJECT

private slots:
    void roles();
    void nothingTyped();
    void everyWordAcrossFields();
    void unicodeCase();
    void rowsSayWhatTheyAre();
    void tabsAcrossGroups();
    void activeTabNotListed();
    void rankedByMatch();
    void frecencyWeights();
    void rankedByFrecency();
    void capped();
    void onePageOneRow();
    void learntFirst();
    void learningFollowsTheHistory();
    void sourcesSwitchedOff();
    void bookmarksWhenEmpty();
    void rebuildsOnSourceChange();
    void quietWhileNothingIsAsked();
    void changesInPlace();
};

namespace {

const qint64 Minute = qint64(60) * 1000;
const qint64 Day = Minute * 60 * 24;

// Everything the omnibar searches, over one temporary directory.
struct Sources
{
    QTemporaryDir dir;
    Storage storage{dir.path()};
    TabModel tabs{nullptr};
    BookmarkModel bookmarks{storage};
    HistoryModel history{storage};
    DownloadModel downloads{storage, dir.path()};
    Settings settings{dir.path() + QStringLiteral("/salama.conf")};
    OmnibarModel omnibar{&tabs, &bookmarks, &history, &downloads, &settings};
};

QVariant role(const OmnibarModel &model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

// Each row as "kind: title", top to bottom.
QStringList rows(const OmnibarModel &model)
{
    QStringList rows;
    for (int row = 0; row < model.rowCount(); ++row) {
        rows.append(role(model, row, OmnibarModel::KindRole).toString() + QStringLiteral(": ") +
                    role(model, row, OmnibarModel::TitleRole).toString());
    }
    return rows;
}

// The first row of a kind, or -1.
int rowOfKind(const OmnibarModel &model, const QString &kind)
{
    for (int row = 0; row < model.rowCount(); ++row) {
        if (role(model, row, OmnibarModel::KindRole).toString() == kind) {
            return row;
        }
    }
    return -1;
}

int countOfKind(const OmnibarModel &model, const QString &kind)
{
    int count = 0;
    for (int row = 0; row < model.rowCount(); ++row) {
        count += role(model, row, OmnibarModel::KindRole).toString() == kind ? 1 : 0;
    }
    return count;
}

// A page of the history as the table would hold it after this many visits, the last
// this long ago.
bool addHistory(const Storage &storage, const QString &url, const QString &title, int visits,
                qint64 age)
{
    QSqlQuery insert(storage.database());
    insert.prepare(QStringLiteral(
        "INSERT INTO browser_history (url, title, visited_count, date) VALUES (?, ?, ?, ?)"));
    insert.addBindValue(url);
    insert.addBindValue(Storage::text(title));
    insert.addBindValue(visits);
    insert.addBindValue(QDateTime::currentMSecsSinceEpoch() - age);
    return insert.exec();
}

// A download as the engine starts one; returns its row's lasting id.
int startDownload(DownloadModel &model, int engineId, const QString &name, const QString &url)
{
    model.observe(model.topic(), QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-start")},
                                             {QStringLiteral("id"), static_cast<double>(engineId)},
                                             {QStringLiteral("displayName"), name},
                                             {QStringLiteral("sourceUrl"), url},
                                             {QStringLiteral("targetPath"), QString()}});
    return model.data(model.index(0, 0), DownloadModel::DownloadIdRole).toInt();
}

void downloadMessage(DownloadModel &model, int engineId, const QVariantMap &extra)
{
    QVariantMap message = extra;
    message.insert(QStringLiteral("id"), static_cast<double>(engineId));
    model.observe(model.topic(), message);
}

// A rebuild a source asked for happens once the event loop comes round.
void settle()
{
    QTest::qWait(10);
}

} // namespace

void tst_omnibarmodel::roles()
{
    Sources sources;
    const QHash<int, QByteArray> names = sources.omnibar.roleNames();
    // QML binds these by name: none may move.
    const QList<QByteArray> expected{
        "kind",       "title",          "url",       "host",          "favicon",
        "tabId",      "groupId",        "groupName", "groupTabCount", "bookmarked",
        "downloadId", "downloadStatus", "progress",  "date",
    };
    QCOMPARE(names.count(), expected.count());
    for (const QByteArray &name : expected) {
        QVERIFY2(names.values().contains(name), name.constData());
    }
    QCOMPARE(names.value(OmnibarModel::KindRole), QByteArray("kind"));
    QCOMPARE(names.value(OmnibarModel::DownloadStatusRole), QByteArray("downloadStatus"));

    QCOMPARE(sources.omnibar.rowCount(), 0);
    QCOMPARE(sources.omnibar.rowCount(sources.omnibar.index(0, 0)), 0);
    QVERIFY(!role(sources.omnibar, 0, OmnibarModel::KindRole).isValid());
    QVERIFY(!role(sources.omnibar, -1, OmnibarModel::KindRole).isValid());

    sources.bookmarks.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    sources.omnibar.setQuery(QStringLiteral("a"));
    QCOMPARE(sources.omnibar.count(), 1);
    QVERIFY(!role(sources.omnibar, 0, Qt::DisplayRole).isValid());
    QCOMPARE(sources.omnibar.rowCount(sources.omnibar.index(0, 0)), 0);
}

// Nothing typed lists nothing, and blanks are nothing typed.
void tst_omnibarmodel::nothingTyped()
{
    Sources sources;
    sources.tabs.newTab(QStringLiteral("https://tab.example/"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://bookmark.example/"), QStringLiteral("B"));
    sources.history.visit(QStringLiteral("https://history.example/"));
    startDownload(sources.downloads, 1, QStringLiteral("a.pdf"),
                  QStringLiteral("https://files.example/a.pdf"));
    QSignalSpy querySpy(&sources.omnibar, &OmnibarModel::queryChanged);
    QSignalSpy resultsSpy(&sources.omnibar, &OmnibarModel::resultsChanged);

    OmnibarModel &omnibar = sources.omnibar;
    QVERIFY(omnibar.query().isEmpty());
    QVERIFY(!omnibar.bookmarksWhenEmpty());
    QCOMPARE(omnibar.count(), 0);

    omnibar.setQuery(QStringLiteral("   "));
    QCOMPARE(querySpy.count(), 0);
    QCOMPARE(omnibar.count(), 0);

    omnibar.setQuery(QStringLiteral("  example "));
    QCOMPARE(omnibar.query(), QStringLiteral("example"));
    QCOMPARE(querySpy.count(), 1);
    QCOMPARE(omnibar.count(), 4);
    QCOMPARE(resultsSpy.count(), 1);
    omnibar.setQuery(QStringLiteral("example"));
    QCOMPARE(querySpy.count(), 1);

    omnibar.setQuery(QString());
    QCOMPARE(querySpy.count(), 2);
    QCOMPARE(omnibar.count(), 0);
    QCOMPARE(resultsSpy.count(), 2);
}

// Every word in one field or another, in every source; one word missing is no match.
// A host that begins with the first word first, then the rest by frecency: the
// bookmark, which weighs twice a page, before the tab.
void tst_omnibarmodel::everyWordAcrossFields()
{
    Sources sources;
    const int tab = sources.tabs.newTab(QStringLiteral("https://yle.fi/uutiset"));
    sources.tabs.updateTitle(tab, QStringLiteral("Helsinki news"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://hs.fi/helsinki"), QStringLiteral("Daily news"));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://news.example/helsinki"),
                       QStringLiteral("World"), 1, Day));
    startDownload(sources.downloads, 1, QStringLiteral("helsinki-map.pdf"),
                  QStringLiteral("https://news.example/files/map.pdf"));
    // Only one of the two words: listed nowhere.
    sources.bookmarks.add(QStringLiteral("https://tampere.fi/"), QStringLiteral("Tampere news"));

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("NEWS helsinki"));
    QCOMPARE(rows(omnibar), (QStringList{
                                QStringLiteral("history: World"),
                                QStringLiteral("bookmark: Daily news"),
                                QStringLiteral("tab: Helsinki news"),
                                QStringLiteral("download: helsinki-map.pdf"),
                            }));

    omnibar.setQuery(QStringLiteral("helsinki weather"));
    QCOMPARE(omnibar.count(), 0);
}

// Unicode's case, in every source.
void tst_omnibarmodel::unicodeCase()
{
    Sources sources;
    const int tab = sources.tabs.newTab(QStringLiteral("https://a.example/"));
    sources.tabs.updateTitle(tab, QStringLiteral("Älypuhelimet"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://b.example/"), QStringLiteral("ÄLYKELLO"));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://c.example/"),
                       QStringLiteral("Älytelevisio"), 1, Day));
    startDownload(sources.downloads, 1, QStringLiteral("Älykoti.pdf"),
                  QStringLiteral("https://d.example/koti.pdf"));

    sources.omnibar.setQuery(QStringLiteral("äly"));
    QCOMPARE(rows(sources.omnibar), (QStringList{
                                        QStringLiteral("bookmark: ÄLYKELLO"),
                                        QStringLiteral("tab: Älypuhelimet"),
                                        QStringLiteral("history: Älytelevisio"),
                                        QStringLiteral("download: Älykoti.pdf"),
                                    }));
}

// What each kind of row carries, and what it leaves empty.
void tst_omnibarmodel::rowsSayWhatTheyAre()
{
    Sources sources;
    const int tab = sources.tabs.newTab(QStringLiteral("https://www.row.example/tab"));
    sources.tabs.updateFavicon(tab, QStringLiteral("https://row.example/tab.ico"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://row.example/bookmark"),
                          QStringLiteral("Bookmarked row"),
                          QStringLiteral("https://row.example/b.ico"));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://row.example/history"), QString(), 3,
                       Day));
    const int download = startDownload(sources.downloads, 7, QStringLiteral("row.pdf"),
                                       QStringLiteral("https://files.row.example/row.pdf"));
    downloadMessage(sources.downloads, 7,
                    {{QStringLiteral("msg"), QStringLiteral("dl-progress")},
                     {QStringLiteral("percent"), 40.0}});

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("row"));
    QCOMPARE(omnibar.count(), 4);

    // The tab: no title yet, so its address stands for it.
    const int tabRow = rowOfKind(omnibar, QStringLiteral("tab"));
    QCOMPARE(role(omnibar, tabRow, OmnibarModel::TitleRole).toString(),
             QStringLiteral("https://www.row.example/tab"));
    QCOMPARE(role(omnibar, tabRow, OmnibarModel::UrlRole).toString(),
             QStringLiteral("https://www.row.example/tab"));
    QCOMPARE(role(omnibar, tabRow, OmnibarModel::HostRole).toString(),
             QStringLiteral("row.example"));
    QCOMPARE(role(omnibar, tabRow, OmnibarModel::FaviconRole).toString(),
             QStringLiteral("https://row.example/tab.ico"));
    QCOMPARE(role(omnibar, tabRow, OmnibarModel::TabIdRole).toInt(), tab);
    QCOMPARE(role(omnibar, tabRow, OmnibarModel::GroupIdRole).toInt(),
             sources.tabs.defaultGroupId());
    QCOMPARE(role(omnibar, tabRow, OmnibarModel::GroupTabCountRole).toInt(), 2);
    QVERIFY(!role(omnibar, tabRow, OmnibarModel::BookmarkedRole).toBool());
    QCOMPARE(role(omnibar, tabRow, OmnibarModel::DownloadIdRole).toInt(), 0);
    QVERIFY(!role(omnibar, tabRow, OmnibarModel::DateRole).toDateTime().isValid());

    const int bookmarkRow = rowOfKind(omnibar, QStringLiteral("bookmark"));
    QCOMPARE(role(omnibar, bookmarkRow, OmnibarModel::TitleRole).toString(),
             QStringLiteral("Bookmarked row"));
    QCOMPARE(role(omnibar, bookmarkRow, OmnibarModel::UrlRole).toString(),
             QStringLiteral("https://row.example/bookmark"));
    QCOMPARE(role(omnibar, bookmarkRow, OmnibarModel::FaviconRole).toString(),
             QStringLiteral("https://row.example/b.ico"));
    QVERIFY(role(omnibar, bookmarkRow, OmnibarModel::BookmarkedRole).toBool());
    QCOMPARE(role(omnibar, bookmarkRow, OmnibarModel::TabIdRole).toInt(), 0);
    QCOMPARE(role(omnibar, bookmarkRow, OmnibarModel::GroupIdRole).toInt(), 0);
    QVERIFY(role(omnibar, bookmarkRow, OmnibarModel::GroupNameRole).toString().isEmpty());
    QCOMPARE(role(omnibar, bookmarkRow, OmnibarModel::DownloadIdRole).toInt(), 0);
    QVERIFY(!role(omnibar, bookmarkRow, OmnibarModel::DateRole).toDateTime().isValid());

    // A page of the history with no title shows its address, and when it was visited.
    const int historyRow = rowOfKind(omnibar, QStringLiteral("history"));
    QCOMPARE(role(omnibar, historyRow, OmnibarModel::TitleRole).toString(),
             QStringLiteral("https://row.example/history"));
    QCOMPARE(role(omnibar, historyRow, OmnibarModel::HostRole).toString(),
             QStringLiteral("row.example"));
    QVERIFY(role(omnibar, historyRow, OmnibarModel::FaviconRole).toString().isEmpty());
    QVERIFY(!role(omnibar, historyRow, OmnibarModel::BookmarkedRole).toBool());
    const QDateTime visited = role(omnibar, historyRow, OmnibarModel::DateRole).toDateTime();
    QVERIFY(visited.isValid());
    QVERIFY(qAbs(visited.msecsTo(QDateTime::currentDateTime()) - Day) < Minute);

    // The download, last: its own lasting id, where it came from, how far it has got.
    QCOMPARE(rowOfKind(omnibar, QStringLiteral("download")), 3);
    QCOMPARE(role(omnibar, 3, OmnibarModel::TitleRole).toString(), QStringLiteral("row.pdf"));
    QCOMPARE(role(omnibar, 3, OmnibarModel::UrlRole).toString(),
             QStringLiteral("https://files.row.example/row.pdf"));
    QCOMPARE(role(omnibar, 3, OmnibarModel::HostRole).toString(),
             QStringLiteral("files.row.example"));
    QCOMPARE(role(omnibar, 3, OmnibarModel::DownloadIdRole).toInt(), download);
    QCOMPARE(sources.downloads.rowOf(download), 0);
    QCOMPARE(role(omnibar, 3, OmnibarModel::DownloadStatusRole).toInt(),
             static_cast<int>(DownloadModel::Running));
    QCOMPARE(role(omnibar, 3, OmnibarModel::ProgressRole).toInt(), 40);
    QCOMPARE(role(omnibar, 3, OmnibarModel::TabIdRole).toInt(), 0);
    QVERIFY(role(omnibar, 3, OmnibarModel::DateRole).toDateTime().isValid());

    // A download with no name at all is called by its address.
    startDownload(sources.downloads, 8, QString(), QStringLiteral("https://row.example/x"));
    omnibar.setQuery(QStringLiteral("row.example/x"));
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("download: https://row.example/x")});
}

// Tabs of every group are found, each with the group it is in.
void tst_omnibarmodel::tabsAcrossGroups()
{
    Sources sources;
    TabModel &tabs = sources.tabs;
    const int home = tabs.newTab(QStringLiteral("https://mail.example/"));
    tabs.updateTitle(home, QStringLiteral("Home mail"));
    const int work = tabs.addGroup(QStringLiteral("Work"));
    const int office = tabs.newTab(QStringLiteral("https://mail.work.example/"));
    tabs.updateTitle(office, QStringLiteral("Office mail"));
    tabs.newTab(QStringLiteral("https://front.example/"));

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("mail"));
    // Both hosts begin with the word, and both are as fresh: the one in front last
    // comes first.
    QCOMPARE(rows(omnibar),
             (QStringList{QStringLiteral("tab: Office mail"), QStringLiteral("tab: Home mail")}));
    QCOMPARE(role(omnibar, 0, OmnibarModel::TabIdRole).toInt(), office);
    QCOMPARE(role(omnibar, 0, OmnibarModel::GroupIdRole).toInt(), work);
    QCOMPARE(role(omnibar, 0, OmnibarModel::GroupNameRole).toString(), QStringLiteral("Work"));
    QCOMPARE(role(omnibar, 0, OmnibarModel::GroupTabCountRole).toInt(), 2);
    QCOMPARE(role(omnibar, 1, OmnibarModel::TabIdRole).toInt(), home);
    QCOMPARE(role(omnibar, 1, OmnibarModel::GroupIdRole).toInt(), tabs.defaultGroupId());
    QVERIFY(role(omnibar, 1, OmnibarModel::GroupNameRole).toString().isEmpty());
    QCOMPARE(role(omnibar, 1, OmnibarModel::GroupTabCountRole).toInt(), 1);

    // A group renamed is a row changed.
    tabs.renameGroup(work, QStringLiteral("Office"));
    QTRY_COMPARE(role(omnibar, 0, OmnibarModel::GroupNameRole).toString(),
                 QStringLiteral("Office"));
}

// The tab in front is the page the bar is over: never a suggestion, whatever it holds.
void tst_omnibarmodel::activeTabNotListed()
{
    Sources sources;
    TabModel &tabs = sources.tabs;
    const int first = tabs.newTab(QStringLiteral("https://one.example/"));
    const int second = tabs.newTab(QStringLiteral("https://two.example/"));

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("example"));
    QCOMPARE(omnibar.count(), 1);
    QCOMPARE(role(omnibar, 0, OmnibarModel::TabIdRole).toInt(), first);

    // Another tab to the front: the list follows.
    tabs.activateTabById(first);
    QTRY_COMPARE(role(omnibar, 0, OmnibarModel::TabIdRole).toInt(), second);
    QCOMPARE(omnibar.count(), 1);
}

// First the hosts that begin with the first word, then the titles and addresses with a
// word that does, then the rest; within each, frecency, and between equals the order
// the sources have them in.
void tst_omnibarmodel::rankedByMatch()
{
    Sources sources;
    BookmarkModel &bookmarks = sources.bookmarks;
    bookmarks.add(QStringLiteral("https://a.example/?q=goodnews"), QStringLiteral("Goodnews"));
    bookmarks.add(QStringLiteral("https://paper.example/news"), QStringLiteral("Evening"));
    bookmarks.add(QStringLiteral("https://news.example/"), QStringLiteral("Daily paper"));
    bookmarks.add(QStringLiteral("https://www.newsroom.example/"), QString());
    bookmarks.add(QStringLiteral("https://b.example/"), QStringLiteral("Morning (news)"));

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("news"));
    QCOMPARE(rows(omnibar), (QStringList{
                                QStringLiteral("bookmark: Daily paper"),
                                QStringLiteral("bookmark: https://www.newsroom.example/"),
                                QStringLiteral("bookmark: Evening"),
                                QStringLiteral("bookmark: Morning (news)"),
                                QStringLiteral("bookmark: Goodnews"),
                            }));

    // A host that begins with the word ahead of a page visited far more often.
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://often.example/news"),
                       QStringLiteral("Often"), 50, Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://news.seldom.example/"),
                       QStringLiteral("Seldom"), 1, 100 * Day));
    omnibar.setQuery(QStringLiteral("news example"));
    const QStringList found = rows(omnibar);
    QVERIFY(found.indexOf(QStringLiteral("history: Seldom")) >= 0);
    QVERIFY(found.indexOf(QStringLiteral("history: Seldom")) <
            found.indexOf(QStringLiteral("history: Often")));
    // And the one visited often ahead of the bookmarks that match as well.
    QVERIFY(found.indexOf(QStringLiteral("history: Often")) <
            found.indexOf(QStringLiteral("bookmark: Evening")));
}

// Firefox's: the day the score wears down to 1. Doubling the score is one half-life on;
// a day older is a day less.
void tst_omnibarmodel::frecencyWeights()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 today = now / Day;
    // 50 is worn down to 1 in 30 * log2(50) days, 169 and a bit.
    QCOMPARE(OmnibarModel::frecency(1, false, now, now), today + 169);
    QCOMPARE(OmnibarModel::frecency(2, false, now, now), today + 199);
    QCOMPARE(OmnibarModel::frecency(1, true, now, now), today + 199);
    QCOMPARE(OmnibarModel::frecency(4, true, now, now), today + 259);
    QCOMPARE(OmnibarModel::frecency(1, false, now - Day, now), today + 168);
    QCOMPARE(OmnibarModel::frecency(2, false, now - 30 * Day, now), today + 169);
    QCOMPARE(OmnibarModel::frecency(1, false, now - 3650 * Day, now), today - 3650 + 169);
    // Never visited is visited once; a use stamped ahead of the clock is one today.
    QCOMPARE(OmnibarModel::frecency(0, false, now, now), today + 169);
    QCOMPARE(OmnibarModel::frecency(1, false, now + 5 * Day, now), today + 169);
}

// Pages that match as well by visits weighed by their age.
void tst_omnibarmodel::rankedByFrecency()
{
    Sources sources;
    const Storage &storage = sources.storage;
    QVERIFY(addHistory(storage, QStringLiteral("https://h1.test/"), QStringLiteral("Page 1"), 1,
                       Day)); // 168
    QVERIFY(addHistory(storage, QStringLiteral("https://h2.test/"), QStringLiteral("Page 2"), 2,
                       10 * Day)); // 189
    QVERIFY(addHistory(storage, QStringLiteral("https://h3.test/"), QStringLiteral("Page 3"), 3,
                       20 * Day)); // 196
    QVERIFY(addHistory(storage, QStringLiteral("https://h4.test/"), QStringLiteral("Page 4"), 10,
                       200 * Day)); // 68
    QVERIFY(addHistory(storage, QStringLiteral("https://h5.test/"), QStringLiteral("Page 5"), 4,
                       60 * Day)); // 169
    QVERIFY(addHistory(storage, QStringLiteral("https://h6.test/"), QStringLiteral("Page 6"), 1,
                       2 * Day)); // 167

    sources.omnibar.setQuery(QStringLiteral("page"));
    QCOMPARE(rows(sources.omnibar), (QStringList{
                                        QStringLiteral("history: Page 3"),
                                        QStringLiteral("history: Page 2"),
                                        QStringLiteral("history: Page 5"),
                                        QStringLiteral("history: Page 1"),
                                        QStringLiteral("history: Page 6"),
                                        QStringLiteral("history: Page 4"),
                                    }));
}

// Eight rows at most, two of them downloads at most, and those last.
void tst_omnibarmodel::capped()
{
    QCOMPARE(OmnibarModel::MaxRows, 8);
    QCOMPARE(OmnibarModel::MaxDownloads, 2);
    QCOMPARE(OmnibarModel::MaxLearnt, 3);

    Sources sources;
    for (int i = 0; i < 12; ++i) {
        sources.tabs.newTab(QStringLiteral("https://cap-tab-%1.example/").arg(i));
        sources.bookmarks.add(QStringLiteral("https://cap-bookmark-%1.example/").arg(i),
                              QStringLiteral("Bookmark %1").arg(i));
        QVERIFY(addHistory(sources.storage,
                           QStringLiteral("https://cap-history-%1.example/").arg(i),
                           QStringLiteral("History %1").arg(i), 1, i * Minute));
    }
    for (int i = 0; i < 7; ++i) {
        startDownload(sources.downloads, i + 1, QStringLiteral("cap-%1.pdf").arg(i),
                      QStringLiteral("https://files.example/cap-%1.pdf").arg(i));
    }
    sources.tabs.newTab(QStringLiteral("https://front.example/"));

    OmnibarModel &omnibar = sources.omnibar;
    QSignalSpy resultsSpy(&omnibar, &OmnibarModel::resultsChanged);
    omnibar.setQuery(QStringLiteral("cap"));
    QCOMPARE(resultsSpy.count(), 1);
    QCOMPARE(omnibar.count(), 8);
    QCOMPARE(omnibar.rowCount(), 8);
    // Every host begins with the word: the bookmarks weigh most, in their order, and
    // the newest two files follow.
    for (int i = 0; i < 6; ++i) {
        QCOMPARE(role(omnibar, i, OmnibarModel::TitleRole).toString(),
                 QStringLiteral("Bookmark %1").arg(i));
    }
    QCOMPARE(role(omnibar, 6, OmnibarModel::TitleRole).toString(), QStringLiteral("cap-6.pdf"));
    QCOMPARE(role(omnibar, 7, OmnibarModel::TitleRole).toString(), QStringLiteral("cap-5.pdf"));

    // One file matching leaves the pages seven.
    omnibar.setQuery(QStringLiteral("cap 1"));
    QCOMPARE(omnibar.count(), 8);
    QCOMPARE(countOfKind(omnibar, QStringLiteral("download")), 1);
    QCOMPARE(rows(omnibar).last(), QStringLiteral("download: cap-1.pdf"));
}

// One row a page, whichever sources hold it: an open tab to switch to before a
// bookmark before a page of the history. Its visits count however its row is listed,
// and so does its being bookmarked.
void tst_omnibarmodel::onePageOneRow()
{
    Sources sources;
    sources.tabs.newTab(QStringLiteral("https://shared.example/"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://shared.example/"), QStringLiteral("Shared"));
    // The page in front is not listed as a tab, so its bookmark is.
    sources.bookmarks.add(QStringLiteral("https://front.example/"), QStringLiteral("Front"));
    sources.bookmarks.add(QStringLiteral("https://bookmark.example/"), QStringLiteral("Mark"));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://shared.example/"),
                       QStringLiteral("Shared page"), 1, Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://bookmark.example/"),
                       QStringLiteral("Marked page"), 1, Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://front.example/"),
                       QStringLiteral("Front page"), 1, Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://history.example/"),
                       QStringLiteral("Old page"), 1, Day));

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("example"));
    // The bookmarks, added today, are fresher than the tab whose last visit was
    // yesterday; the bookmarked tab weighs more than the page no one bookmarked.
    QCOMPARE(rows(omnibar), (QStringList{
                                QStringLiteral("bookmark: Front"),
                                QStringLiteral("bookmark: Mark"),
                                QStringLiteral("tab: https://shared.example/"),
                                QStringLiteral("history: Old page"),
                            }));
    QVERIFY(role(omnibar, 2, OmnibarModel::BookmarkedRole).toBool());
    QVERIFY(!role(omnibar, 3, OmnibarModel::BookmarkedRole).toBool());

    // With the tabs switched off the shared page is its bookmark.
    sources.settings.setOmnibarTabs(false);
    QTRY_COMPARE(rows(omnibar).first(), QStringLiteral("bookmark: Shared"));
    QCOMPARE(omnibar.count(), 4);
    // And with the bookmarks off too, the history, the bookmarked pages still weighing
    // more.
    sources.settings.setOmnibarBookmarks(false);
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("history")), 4);
    QCOMPARE(rows(omnibar).last(), QStringLiteral("history: Old page"));
    QVERIFY(role(omnibar, 0, OmnibarModel::BookmarkedRole).toBool());
}

// What was chosen after what is typed now comes first, even when the words are not in
// it -- no more than three such -- the likeliest first.
void tst_omnibarmodel::learntFirst()
{
    Sources sources;
    OmnibarModel &omnibar = sources.omnibar;
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://github.com/"),
                       QStringLiteral("GitHub"), 1, 10 * Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://ghost.example/"),
                       QStringLiteral("Ghost"), 20, Day));
    omnibar.setQuery(QStringLiteral("gh"));
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("history: Ghost")});

    omnibar.learn(QStringLiteral(" GH "), QStringLiteral("https://github.com/"));
    omnibar.setQuery(QString());
    omnibar.setQuery(QStringLiteral("gh"));
    QCOMPARE(rows(omnibar),
             (QStringList{QStringLiteral("history: GitHub"), QStringLiteral("history: Ghost")}));
    // Less typed than was learnt still leads there; more does not.
    omnibar.setQuery(QStringLiteral("g"));
    QCOMPARE(rows(omnibar).first(), QStringLiteral("history: GitHub"));
    omnibar.setQuery(QStringLiteral("ghx"));
    QCOMPARE(omnibar.count(), 0);

    // What was typed exactly counts twice what only begins with it: "gi", learnt once
    // for Ghost, beats "git", learnt twice for GitHub.
    omnibar.learn(QStringLiteral("git"), QStringLiteral("https://github.com/"));
    omnibar.learn(QStringLiteral("git"), QStringLiteral("https://github.com/"));
    omnibar.learn(QStringLiteral("gi"), QStringLiteral("https://ghost.example/"));
    omnibar.setQuery(QStringLiteral("gi"));
    QCOMPARE(rows(omnibar),
             (QStringList{QStringLiteral("history: Ghost"), QStringLiteral("history: GitHub")}));

    // Three first at most; a fourth learnt takes its place among the rest.
    for (int i = 0; i < 4; ++i) {
        const QString url = QStringLiteral("https://learnt-%1.example/").arg(i);
        QVERIFY(addHistory(sources.storage, url, QStringLiteral("Learnt %1").arg(i), 1, Day));
        for (int times = 0; times <= i; ++times) {
            omnibar.learn(QStringLiteral("zz"), url);
        }
    }
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://zz.example/"), QStringLiteral("Zz"),
                       1, 0));
    omnibar.setQuery(QStringLiteral("zz"));
    QCOMPARE(rows(omnibar), (QStringList{
                                QStringLiteral("history: Learnt 3"),
                                QStringLiteral("history: Learnt 2"),
                                QStringLiteral("history: Learnt 1"),
                                QStringLiteral("history: Zz"),
                                QStringLiteral("history: Learnt 0"),
                            }));
}

// Nothing is learnt while the history is not kept, and what was learnt of a page goes
// with it.
void tst_omnibarmodel::learningFollowsTheHistory()
{
    Sources sources;
    OmnibarModel &omnibar = sources.omnibar;
    sources.history.visit(QStringLiteral("https://kept.example/"), QStringLiteral("Kept"));
    sources.settings.setRememberHistory(false);
    omnibar.learn(QStringLiteral("qq"), QStringLiteral("https://kept.example/"));
    omnibar.setQuery(QStringLiteral("qq"));
    QCOMPARE(omnibar.count(), 0);

    sources.settings.setRememberHistory(true);
    omnibar.learn(QStringLiteral("qq"), QStringLiteral("https://kept.example/"));
    omnibar.setQuery(QString());
    omnibar.setQuery(QStringLiteral("qq"));
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("history: Kept")});

    sources.history.remove(0);
    omnibar.setQuery(QString());
    sources.history.visit(QStringLiteral("https://kept.example/"), QStringLiteral("Kept"));
    omnibar.setQuery(QStringLiteral("qq"));
    QCOMPARE(omnibar.count(), 0);
}

// A source switched off in Settings lists nothing.
void tst_omnibarmodel::sourcesSwitchedOff()
{
    Sources sources;
    sources.tabs.newTab(QStringLiteral("https://off-tab.example/"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://off-bookmark.example/"), QStringLiteral("B"));
    sources.history.visit(QStringLiteral("https://off-history.example/"));
    startDownload(sources.downloads, 1, QStringLiteral("off.pdf"),
                  QStringLiteral("https://files.example/off.pdf"));

    OmnibarModel &omnibar = sources.omnibar;
    Settings &settings = sources.settings;
    omnibar.setQuery(QStringLiteral("off"));
    QCOMPARE(omnibar.count(), 4);

    settings.setOmnibarTabs(false);
    QTRY_COMPARE(omnibar.count(), 3);
    QCOMPARE(countOfKind(omnibar, QStringLiteral("tab")), 0);

    settings.setOmnibarBookmarks(false);
    QTRY_COMPARE(omnibar.count(), 2);
    QCOMPARE(countOfKind(omnibar, QStringLiteral("bookmark")), 0);

    settings.setOmnibarHistory(false);
    QTRY_COMPARE(omnibar.count(), 1);
    QCOMPARE(role(omnibar, 0, OmnibarModel::KindRole).toString(), QStringLiteral("download"));

    settings.setOmnibarDownloads(false);
    QTRY_COMPARE(omnibar.count(), 0);

    settings.setOmnibarHistory(true);
    QTRY_COMPARE(omnibar.count(), 1);
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("history: https://off-history.example/")});
}

// For a new tab, nothing typed lists every bookmark, in their order, and nothing else.
void tst_omnibarmodel::bookmarksWhenEmpty()
{
    Sources sources;
    sources.tabs.newTab(QStringLiteral("https://tab.example/"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.history.visit(QStringLiteral("https://history.example/"));
    for (int i = 0; i < 12; ++i) {
        sources.bookmarks.add(QStringLiteral("https://b%1.example/").arg(i),
                              QStringLiteral("Bookmark %1").arg(i));
    }

    OmnibarModel &omnibar = sources.omnibar;
    QSignalSpy spy(&omnibar, &OmnibarModel::bookmarksWhenEmptyChanged);
    omnibar.setBookmarksWhenEmpty(true);
    QVERIFY(omnibar.bookmarksWhenEmpty());
    QCOMPARE(spy.count(), 1);
    omnibar.setBookmarksWhenEmpty(true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(omnibar.count(), 12);
    QCOMPARE(countOfKind(omnibar, QStringLiteral("bookmark")), 12);
    QCOMPARE(role(omnibar, 0, OmnibarModel::TitleRole).toString(), QStringLiteral("Bookmark 0"));
    QCOMPARE(role(omnibar, 11, OmnibarModel::TitleRole).toString(), QStringLiteral("Bookmark 11"));
    QVERIFY(role(omnibar, 0, OmnibarModel::BookmarkedRole).toBool());

    // Something typed is a search like any other.
    omnibar.setQuery(QStringLiteral("tab"));
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("tab: https://tab.example/")});
    omnibar.setQuery(QString());
    QCOMPARE(omnibar.count(), 12);

    // The bookmarks follow while they are listed.
    sources.bookmarks.remove(0);
    QTRY_COMPARE(role(omnibar, 0, OmnibarModel::TitleRole).toString(),
                 QStringLiteral("Bookmark 1"));
    QCOMPARE(omnibar.count(), 11);

    // The bookmarks switched off in Settings are off here too.
    sources.settings.setOmnibarBookmarks(false);
    QTRY_COMPARE(omnibar.count(), 0);
    sources.settings.setOmnibarBookmarks(true);
    QTRY_COMPARE(omnibar.count(), 11);

    omnibar.setBookmarksWhenEmpty(false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(omnibar.count(), 0);
}

// While there is a query, the list follows every source, and many changes at once are
// one rebuild.
void tst_omnibarmodel::rebuildsOnSourceChange()
{
    Sources sources;
    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("fresh"));
    QCOMPARE(omnibar.count(), 0);
    QSignalSpy resetSpy(&omnibar, &OmnibarModel::modelReset);
    QSignalSpy resultsSpy(&omnibar, &OmnibarModel::resultsChanged);

    // A tab opened, and another in front of it: several signals, one rebuild.
    sources.tabs.newTab(QStringLiteral("https://fresh.example/"));
    const int front = sources.tabs.newTab(QStringLiteral("https://front.example/"));
    QCOMPARE(omnibar.count(), 0);
    settle();
    QCOMPARE(countOfKind(omnibar, QStringLiteral("tab")), 1);
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(resultsSpy.count(), 1);

    sources.bookmarks.add(QStringLiteral("https://fresh-bookmark.example/"), QString());
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("bookmark")), 1);
    sources.history.visit(QStringLiteral("https://fresh-history.example/"));
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("history")), 1);
    startDownload(sources.downloads, 1, QStringLiteral("fresh.pdf"),
                  QStringLiteral("https://files.example/fresh.pdf"));
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("download")), 1);
    QCOMPARE(omnibar.count(), 4);

    // A title arriving can make a match, or change what a row says.
    const int titled = sources.tabs.newTab(QStringLiteral("https://titled.example/"));
    sources.tabs.activateTabById(front);
    settle();
    QCOMPARE(countOfKind(omnibar, QStringLiteral("tab")), 1);
    sources.tabs.updateTitle(titled, QStringLiteral("Fresh news"));
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("tab")), 2);
    sources.bookmarks.edit(0, QStringLiteral("https://fresh-bookmark.example/"),
                           QStringLiteral("Fresh mark"));
    QTRY_VERIFY(rows(omnibar).contains(QStringLiteral("bookmark: Fresh mark")));
    sources.history.updateTitle(QStringLiteral("https://fresh-history.example/"),
                                QStringLiteral("Fresh history"));
    QTRY_VERIFY(rows(omnibar).contains(QStringLiteral("history: Fresh history")));

    // Things going.
    sources.tabs.closeTabById(titled);
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("tab")), 1);
    sources.history.remove(0);
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("history")), 0);
    sources.downloads.remove(0);
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("download")), 0);
    sources.bookmarks.clear();
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("bookmark")), 0);
    QCOMPARE(omnibar.count(), 1);
}

// With nothing asked of it, the model does not rebuild for its sources' sake.
void tst_omnibarmodel::quietWhileNothingIsAsked()
{
    Sources sources;
    OmnibarModel &omnibar = sources.omnibar;
    QSignalSpy resetSpy(&omnibar, &OmnibarModel::modelReset);
    QSignalSpy changeSpy(&omnibar, &OmnibarModel::dataChanged);
    QSignalSpy resultsSpy(&omnibar, &OmnibarModel::resultsChanged);

    sources.tabs.newTab(QStringLiteral("https://quiet.example/"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://quiet.example/"), QStringLiteral("Q"));
    sources.history.visit(QStringLiteral("https://quiet.example/history"));
    sources.settings.setOmnibarTabs(false);
    settle();
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(changeSpy.count(), 0);
    QCOMPARE(resultsSpy.count(), 0);
    QCOMPARE(omnibar.count(), 0);

    // A query cleared while a rebuild waits: the waiting one finds nothing to do.
    omnibar.setQuery(QStringLiteral("quiet"));
    QCOMPARE(omnibar.count(), 2);
    sources.bookmarks.add(QStringLiteral("https://quiet-too.example/"), QStringLiteral("Q2"));
    omnibar.setQuery(QString());
    QCOMPARE(omnibar.count(), 0);
    const int resets = resetSpy.count();
    settle();
    QCOMPARE(resetSpy.count(), resets);
    QCOMPARE(omnibar.count(), 0);
}

// The same rows built again are changed in place, never reset: a download's progress
// or a favicon arriving does not tear the row down under a finger.
void tst_omnibarmodel::changesInPlace()
{
    Sources sources;
    const int tab = sources.tabs.newTab(QStringLiteral("https://report.example/"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    startDownload(sources.downloads, 3, QStringLiteral("report.pdf"),
                  QStringLiteral("https://files.example/report.pdf"));

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("report"));
    QCOMPARE(rows(omnibar), (QStringList{QStringLiteral("tab: https://report.example/"),
                                         QStringLiteral("download: report.pdf")}));
    QSignalSpy resetSpy(&omnibar, &OmnibarModel::modelReset);
    QSignalSpy changeSpy(&omnibar, &OmnibarModel::dataChanged);
    QSignalSpy resultsSpy(&omnibar, &OmnibarModel::resultsChanged);

    downloadMessage(sources.downloads, 3,
                    {{QStringLiteral("msg"), QStringLiteral("dl-progress")},
                     {QStringLiteral("percent"), 50.0}});
    QTRY_COMPARE(role(omnibar, 1, OmnibarModel::ProgressRole).toInt(), 50);
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(changeSpy.count(), 1);
    // Only the row that changed is told.
    QCOMPARE(changeSpy.last().at(0).toModelIndex().row(), 1);
    QCOMPARE(changeSpy.last().at(1).toModelIndex().row(), 1);
    QCOMPARE(resultsSpy.count(), 0);

    downloadMessage(sources.downloads, 3,
                    {{QStringLiteral("msg"), QStringLiteral("dl-done")},
                     {QStringLiteral("targetPath"), QStringLiteral("/tmp/report.pdf")}});
    QTRY_COMPARE(role(omnibar, 1, OmnibarModel::DownloadStatusRole).toInt(),
                 static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(omnibar, 1, OmnibarModel::ProgressRole).toInt(), 100);
    QCOMPARE(resetSpy.count(), 0);

    sources.tabs.updateFavicon(tab, QStringLiteral("https://report.example/favicon.ico"));
    QTRY_COMPARE(role(omnibar, 0, OmnibarModel::FaviconRole).toString(),
                 QStringLiteral("https://report.example/favicon.ico"));
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(changeSpy.last().at(0).toModelIndex().row(), 0);

    // A change that changes nothing the rows say says nothing.
    const int changes = changeSpy.count();
    sources.tabs.updateThumbnail(tab, QStringLiteral("/nowhere/tab.png"));
    settle();
    QCOMPARE(changeSpy.count(), changes);
    QCOMPARE(resetSpy.count(), 0);

    // Different rows are a different list.
    sources.bookmarks.add(QStringLiteral("https://report-bookmark.example/"), QString());
    QTRY_COMPARE(resetSpy.count(), 1);
    QCOMPARE(omnibar.count(), 3);
    QCOMPARE(resultsSpy.count(), 1);
}

QTEST_GUILESS_MAIN(tst_omnibarmodel)
#include "tst_omnibarmodel.moc"
