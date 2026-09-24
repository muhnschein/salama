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
    void rankingIsStable();
    void frecencyWeights();
    void historyByFrecency();
    void capsAndTotals();
    void dedupe();
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
        "kind",           "title",    "url",       "host",          "favicon",
        "tabId",          "groupId",  "groupName", "groupTabCount", "downloadId",
        "downloadStatus", "progress", "date",
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
    QCOMPARE(omnibar.tabCount(), 0);
    QCOMPARE(omnibar.tabTotal(), 0);
    QCOMPARE(omnibar.bookmarkTotal(), 0);
    QCOMPARE(omnibar.historyTotal(), 0);
    QCOMPARE(omnibar.downloadTotal(), 0);
    QCOMPARE(resultsSpy.count(), 2);
}

// Every word in one field or another, in every source; one word missing is no match.
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
                                QStringLiteral("tab: Helsinki news"),
                                QStringLiteral("bookmark: Daily news"),
                                QStringLiteral("history: World"),
                                QStringLiteral("download: helsinki-map.pdf"),
                            }));
    QCOMPARE(omnibar.tabCount(), 1);
    QCOMPARE(omnibar.bookmarkCount(), 1);
    QCOMPARE(omnibar.historyCount(), 1);
    QCOMPARE(omnibar.downloadCount(), 1);

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
                                        QStringLiteral("tab: Älypuhelimet"),
                                        QStringLiteral("bookmark: ÄLYKELLO"),
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
    QCOMPARE(role(omnibar, 0, OmnibarModel::KindRole).toString(), QStringLiteral("tab"));
    QCOMPARE(role(omnibar, 0, OmnibarModel::TitleRole).toString(),
             QStringLiteral("https://www.row.example/tab"));
    QCOMPARE(role(omnibar, 0, OmnibarModel::UrlRole).toString(),
             QStringLiteral("https://www.row.example/tab"));
    QCOMPARE(role(omnibar, 0, OmnibarModel::HostRole).toString(), QStringLiteral("row.example"));
    QCOMPARE(role(omnibar, 0, OmnibarModel::FaviconRole).toString(),
             QStringLiteral("https://row.example/tab.ico"));
    QCOMPARE(role(omnibar, 0, OmnibarModel::TabIdRole).toInt(), tab);
    QCOMPARE(role(omnibar, 0, OmnibarModel::GroupIdRole).toInt(), sources.tabs.defaultGroupId());
    QCOMPARE(role(omnibar, 0, OmnibarModel::GroupTabCountRole).toInt(), 2);
    QCOMPARE(role(omnibar, 0, OmnibarModel::DownloadIdRole).toInt(), 0);
    QVERIFY(!role(omnibar, 0, OmnibarModel::DateRole).toDateTime().isValid());

    QCOMPARE(role(omnibar, 1, OmnibarModel::KindRole).toString(), QStringLiteral("bookmark"));
    QCOMPARE(role(omnibar, 1, OmnibarModel::TitleRole).toString(),
             QStringLiteral("Bookmarked row"));
    QCOMPARE(role(omnibar, 1, OmnibarModel::UrlRole).toString(),
             QStringLiteral("https://row.example/bookmark"));
    QCOMPARE(role(omnibar, 1, OmnibarModel::FaviconRole).toString(),
             QStringLiteral("https://row.example/b.ico"));
    QCOMPARE(role(omnibar, 1, OmnibarModel::TabIdRole).toInt(), 0);
    QCOMPARE(role(omnibar, 1, OmnibarModel::GroupIdRole).toInt(), 0);
    QVERIFY(role(omnibar, 1, OmnibarModel::GroupNameRole).toString().isEmpty());
    QCOMPARE(role(omnibar, 1, OmnibarModel::DownloadIdRole).toInt(), 0);
    QVERIFY(!role(omnibar, 1, OmnibarModel::DateRole).toDateTime().isValid());

    // A page of the history with no title shows its address, and when it was visited.
    QCOMPARE(role(omnibar, 2, OmnibarModel::KindRole).toString(), QStringLiteral("history"));
    QCOMPARE(role(omnibar, 2, OmnibarModel::TitleRole).toString(),
             QStringLiteral("https://row.example/history"));
    QCOMPARE(role(omnibar, 2, OmnibarModel::HostRole).toString(), QStringLiteral("row.example"));
    QVERIFY(role(omnibar, 2, OmnibarModel::FaviconRole).toString().isEmpty());
    const QDateTime visited = role(omnibar, 2, OmnibarModel::DateRole).toDateTime();
    QVERIFY(visited.isValid());
    QVERIFY(qAbs(visited.msecsTo(QDateTime::currentDateTime()) - Day) < Minute);

    // The download: its own lasting id, where it came from, how far it has got.
    QCOMPARE(role(omnibar, 3, OmnibarModel::KindRole).toString(), QStringLiteral("download"));
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
    // Both hosts begin with the word; the one in front last comes first.
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
    QCOMPARE(omnibar.tabCount(), 2);
    QCOMPARE(omnibar.tabTotal(), 2);

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
    QCOMPARE(omnibar.tabCount(), 1);
    QCOMPARE(omnibar.tabTotal(), 1);
    QCOMPARE(role(omnibar, 0, OmnibarModel::TabIdRole).toInt(), first);

    // Another tab to the front: the list follows.
    tabs.activateTabById(first);
    QTRY_COMPARE(role(omnibar, 0, OmnibarModel::TabIdRole).toInt(), second);
    QCOMPARE(omnibar.tabCount(), 1);
}

// First the hosts that begin with the first word, then the titles with a word that
// does, then the rest; within each, the order the source has them in.
void tst_omnibarmodel::rankingIsStable()
{
    Sources sources;
    BookmarkModel &bookmarks = sources.bookmarks;
    bookmarks.add(QStringLiteral("https://a.example/?q=goodnews"), QStringLiteral("Goodnews"));
    bookmarks.add(QStringLiteral("https://paper.example/news"), QStringLiteral("Evening News"));
    bookmarks.add(QStringLiteral("https://news.example/"), QStringLiteral("Daily paper"));
    bookmarks.add(QStringLiteral("https://www.newsroom.example/"), QString());
    bookmarks.add(QStringLiteral("https://b.example/news"), QStringLiteral("Morning (news)"));

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("news"));
    QCOMPARE(rows(omnibar), (QStringList{
                                QStringLiteral("bookmark: Daily paper"),
                                QStringLiteral("bookmark: https://www.newsroom.example/"),
                                QStringLiteral("bookmark: Evening News"),
                                QStringLiteral("bookmark: Morning (news)"),
                                QStringLiteral("bookmark: Goodnews"),
                            }));

    // Tabs: rank before recency. The one in front most recently is behind the one
    // whose host begins with the word.
    const int host = sources.tabs.newTab(QStringLiteral("https://news.tabs.example/"));
    const int title = sources.tabs.newTab(QStringLiteral("https://tabs.example/1"));
    sources.tabs.updateTitle(title, QStringLiteral("All the news"));
    const int other = sources.tabs.newTab(QStringLiteral("https://tabs.example/goodnews"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    omnibar.setQuery(QStringLiteral("news tabs"));
    QCOMPARE(omnibar.tabCount(), 3);
    QCOMPARE(role(omnibar, 0, OmnibarModel::TabIdRole).toInt(), host);
    QCOMPARE(role(omnibar, 1, OmnibarModel::TabIdRole).toInt(), title);
    QCOMPARE(role(omnibar, 2, OmnibarModel::TabIdRole).toInt(), other);

    // History: rank before frecency.
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://often.example/news"),
                       QStringLiteral("Often"), 50, Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://news.seldom.example/"),
                       QStringLiteral("Seldom"), 1, 100 * Day));
    omnibar.setQuery(QStringLiteral("news example"));
    const int firstHistory = omnibar.tabCount() + omnibar.bookmarkCount();
    QCOMPARE(omnibar.historyCount(), 2);
    QCOMPARE(role(omnibar, firstHistory, OmnibarModel::TitleRole).toString(),
             QStringLiteral("Seldom"));
    QCOMPARE(role(omnibar, firstHistory + 1, OmnibarModel::TitleRole).toString(),
             QStringLiteral("Often"));
}

void tst_omnibarmodel::frecencyWeights()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto at = [now](qint64 age) { return QDateTime::fromMSecsSinceEpoch(now - age); };
    QCOMPARE(OmnibarModel::frecency(1, at(0), now), qint64(100));
    QCOMPARE(OmnibarModel::frecency(3, at(4 * Day), now), qint64(300));
    QCOMPARE(OmnibarModel::frecency(3, at(4 * Day + 1), now), qint64(210));
    QCOMPARE(OmnibarModel::frecency(1, at(14 * Day), now), qint64(70));
    QCOMPARE(OmnibarModel::frecency(1, at(14 * Day + 1), now), qint64(50));
    QCOMPARE(OmnibarModel::frecency(1, at(31 * Day), now), qint64(50));
    QCOMPARE(OmnibarModel::frecency(1, at(31 * Day + 1), now), qint64(30));
    QCOMPARE(OmnibarModel::frecency(1, at(90 * Day), now), qint64(30));
    QCOMPARE(OmnibarModel::frecency(1, at(90 * Day + 1), now), qint64(10));
    QCOMPARE(OmnibarModel::frecency(2, at(3650 * Day), now), qint64(20));
    // A visit stamped ahead of the clock -- a clock set back -- is a recent one.
    QCOMPARE(OmnibarModel::frecency(1, at(-Day), now), qint64(100));
    QCOMPARE(OmnibarModel::frecency(0, at(0), now), qint64(0));
}

// The history by visits weighed by their age, the later first between equals.
void tst_omnibarmodel::historyByFrecency()
{
    Sources sources;
    const Storage &storage = sources.storage;
    QVERIFY(addHistory(storage, QStringLiteral("https://h1.test/"), QStringLiteral("Page 1"), 1,
                       Day)); // 100
    QVERIFY(addHistory(storage, QStringLiteral("https://h2.test/"), QStringLiteral("Page 2"), 2,
                       10 * Day)); // 140
    QVERIFY(addHistory(storage, QStringLiteral("https://h3.test/"), QStringLiteral("Page 3"), 3,
                       20 * Day)); // 150
    QVERIFY(addHistory(storage, QStringLiteral("https://h4.test/"), QStringLiteral("Page 4"), 10,
                       200 * Day)); // 100
    QVERIFY(addHistory(storage, QStringLiteral("https://h5.test/"), QStringLiteral("Page 5"), 4,
                       60 * Day)); // 120
    QVERIFY(addHistory(storage, QStringLiteral("https://h6.test/"), QStringLiteral("Page 6"), 1,
                       2 * Day)); // 100

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

// Each section shows its cap, and says how many it had.
void tst_omnibarmodel::capsAndTotals()
{
    QCOMPARE(OmnibarModel::TabCap, 10);
    QCOMPARE(OmnibarModel::BookmarkCap, 10);
    QCOMPARE(OmnibarModel::HistoryCap, 10);
    QCOMPARE(OmnibarModel::DownloadCap, 5);

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
    QCOMPARE(omnibar.tabCount(), 10);
    QCOMPARE(omnibar.tabTotal(), 12);
    QCOMPARE(omnibar.bookmarkCount(), 10);
    QCOMPARE(omnibar.bookmarkTotal(), 12);
    QCOMPARE(omnibar.historyCount(), 10);
    QCOMPARE(omnibar.historyTotal(), 12);
    QCOMPARE(omnibar.downloadCount(), 5);
    QCOMPARE(omnibar.downloadTotal(), 7);
    QCOMPARE(omnibar.count(), 35);
    QCOMPARE(omnibar.rowCount(), 35);

    // In section order, each capped from the top of its own order: the tabs most
    // recently in front, the bookmarks as listed, the newest history, the newest files.
    QCOMPARE(role(omnibar, 0, OmnibarModel::UrlRole).toString(),
             QStringLiteral("https://cap-tab-11.example/"));
    QCOMPARE(role(omnibar, 9, OmnibarModel::UrlRole).toString(),
             QStringLiteral("https://cap-tab-2.example/"));
    QCOMPARE(role(omnibar, 10, OmnibarModel::TitleRole).toString(), QStringLiteral("Bookmark 0"));
    QCOMPARE(role(omnibar, 19, OmnibarModel::TitleRole).toString(), QStringLiteral("Bookmark 9"));
    QCOMPARE(role(omnibar, 20, OmnibarModel::TitleRole).toString(), QStringLiteral("History 0"));
    QCOMPARE(role(omnibar, 29, OmnibarModel::TitleRole).toString(), QStringLiteral("History 9"));
    QCOMPARE(role(omnibar, 30, OmnibarModel::TitleRole).toString(), QStringLiteral("cap-6.pdf"));
    QCOMPARE(role(omnibar, 34, OmnibarModel::TitleRole).toString(), QStringLiteral("cap-2.pdf"));

    // Narrowed to under the caps, the counts are the totals.
    omnibar.setQuery(QStringLiteral("cap 1"));
    QCOMPARE(omnibar.tabCount(), 3); // 1, 10, 11
    QCOMPARE(omnibar.tabTotal(), 3);
    QCOMPARE(omnibar.downloadCount(), 1);
    QCOMPARE(omnibar.downloadTotal(), 1);
}

// One row a page: a bookmark open in a listed tab, and a page of the history listed as
// either, are left out -- and not counted.
void tst_omnibarmodel::dedupe()
{
    Sources sources;
    sources.tabs.newTab(QStringLiteral("https://shared.example/"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://shared.example/"), QStringLiteral("Shared"));
    // The page in front is not listed under the tabs, so its bookmark is.
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
    QCOMPARE(rows(omnibar), (QStringList{
                                QStringLiteral("tab: https://shared.example/"),
                                QStringLiteral("bookmark: Front"),
                                QStringLiteral("bookmark: Mark"),
                                QStringLiteral("history: Old page"),
                            }));
    QCOMPARE(omnibar.bookmarkTotal(), 2);
    QCOMPARE(omnibar.historyTotal(), 1);

    // With the tabs switched off nothing is listed as one, and the bookmark is back;
    // its page of the history still is not.
    sources.settings.setOmnibarTabs(false);
    QTRY_COMPARE(omnibar.bookmarkTotal(), 3);
    QCOMPARE(omnibar.historyTotal(), 1);
    // And with the bookmarks off too, the whole history.
    sources.settings.setOmnibarBookmarks(false);
    QTRY_COMPARE(omnibar.historyTotal(), 4);
    QCOMPARE(omnibar.count(), 4);
}

// A source switched off in Settings lists nothing and counts nothing.
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
    QCOMPARE(omnibar.tabCount(), 0);
    QCOMPARE(omnibar.tabTotal(), 0);

    settings.setOmnibarBookmarks(false);
    QTRY_COMPARE(omnibar.count(), 2);
    QCOMPARE(omnibar.bookmarkCount(), 0);
    QCOMPARE(omnibar.bookmarkTotal(), 0);

    settings.setOmnibarHistory(false);
    QTRY_COMPARE(omnibar.count(), 1);
    QCOMPARE(omnibar.historyCount(), 0);
    QCOMPARE(omnibar.historyTotal(), 0);
    QCOMPARE(role(omnibar, 0, OmnibarModel::KindRole).toString(), QStringLiteral("download"));

    settings.setOmnibarDownloads(false);
    QTRY_COMPARE(omnibar.count(), 0);
    QCOMPARE(omnibar.downloadTotal(), 0);

    settings.setOmnibarHistory(true);
    QTRY_COMPARE(omnibar.count(), 1);
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("history: https://off-history.example/")});
}

// For a new tab, nothing typed lists the bookmarks, in their order and capped, and
// nothing else.
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
    QCOMPARE(omnibar.count(), 10);
    QCOMPARE(omnibar.bookmarkCount(), 10);
    QCOMPARE(omnibar.bookmarkTotal(), 12);
    QCOMPARE(omnibar.tabTotal(), 0);
    QCOMPARE(omnibar.historyTotal(), 0);
    QCOMPARE(role(omnibar, 0, OmnibarModel::TitleRole).toString(), QStringLiteral("Bookmark 0"));
    QCOMPARE(role(omnibar, 9, OmnibarModel::TitleRole).toString(), QStringLiteral("Bookmark 9"));

    // Something typed is a search like any other.
    omnibar.setQuery(QStringLiteral("example"));
    QCOMPARE(omnibar.tabCount(), 1);
    QCOMPARE(omnibar.historyCount(), 1);
    omnibar.setQuery(QString());
    QCOMPARE(omnibar.count(), 10);

    // The bookmarks follow while they are listed.
    sources.bookmarks.remove(0);
    QTRY_COMPARE(role(omnibar, 0, OmnibarModel::TitleRole).toString(),
                 QStringLiteral("Bookmark 1"));
    QCOMPARE(omnibar.bookmarkTotal(), 11);

    // The bookmarks switched off in Settings are off here too.
    sources.settings.setOmnibarBookmarks(false);
    QTRY_COMPARE(omnibar.count(), 0);
    sources.settings.setOmnibarBookmarks(true);
    QTRY_COMPARE(omnibar.count(), 10);

    omnibar.setBookmarksWhenEmpty(false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(omnibar.count(), 0);
    QCOMPARE(omnibar.bookmarkTotal(), 0);
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
    QCOMPARE(omnibar.tabCount(), 1);
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(resultsSpy.count(), 1);

    sources.bookmarks.add(QStringLiteral("https://fresh-bookmark.example/"), QString());
    QTRY_COMPARE(omnibar.bookmarkCount(), 1);
    sources.history.visit(QStringLiteral("https://fresh-history.example/"));
    QTRY_COMPARE(omnibar.historyCount(), 1);
    startDownload(sources.downloads, 1, QStringLiteral("fresh.pdf"),
                  QStringLiteral("https://files.example/fresh.pdf"));
    QTRY_COMPARE(omnibar.downloadCount(), 1);
    QCOMPARE(omnibar.count(), 4);

    // A title arriving can make a match, or change what a row says.
    const int titled = sources.tabs.newTab(QStringLiteral("https://titled.example/"));
    sources.tabs.activateTabById(front);
    settle();
    QCOMPARE(omnibar.tabCount(), 1);
    sources.tabs.updateTitle(titled, QStringLiteral("Fresh news"));
    QTRY_COMPARE(omnibar.tabCount(), 2);
    sources.bookmarks.edit(0, QStringLiteral("https://fresh-bookmark.example/"),
                           QStringLiteral("Fresh mark"));
    QTRY_COMPARE(role(omnibar, 2, OmnibarModel::TitleRole).toString(),
                 QStringLiteral("Fresh mark"));
    sources.history.updateTitle(QStringLiteral("https://fresh-history.example/"),
                                QStringLiteral("Fresh history"));
    QTRY_COMPARE(role(omnibar, 3, OmnibarModel::TitleRole).toString(),
                 QStringLiteral("Fresh history"));

    // Things going.
    sources.tabs.closeTabById(titled);
    QTRY_COMPARE(omnibar.tabCount(), 1);
    sources.history.remove(0);
    QTRY_COMPARE(omnibar.historyCount(), 0);
    sources.downloads.remove(0);
    QTRY_COMPARE(omnibar.downloadCount(), 0);
    sources.bookmarks.clear();
    QTRY_COMPARE(omnibar.bookmarkCount(), 0);
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
