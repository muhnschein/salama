// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "bookmarks/BookmarkModel.h"
#include "downloads/DownloadModel.h"
#include "history/HistoryModel.h"
#include "omnibar/OmnibarModel.h"
#include "search/SearchEngines.h"
#include "search/SearchWords.h"
#include "settings/PrivacySettings.h"
#include "settings/SearchSettings.h"
#include "storage/Storage.h"
#include "tabs/TabModel.h"

#include <QDateTime>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

using Salama::BookmarkModel;
using Salama::DownloadModel;
using Salama::HistoryModel;
using Salama::OmnibarKind;
using Salama::OmnibarModel;
using Salama::OmnibarRow;
using Salama::PrivacySettings;
using Salama::SearchEngines;
using Salama::SearchSettings;
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
    void favicons();
    void markedWords();
    void sourcesSwitchedOff();
    void bookmarksWhenEmpty();
    void rebuildsOnSourceChange();
    void quietWhileNothingIsAsked();
    void changesInPlace();
    void openPaneSeesVisits();
    void typingReadsHistoryOnce();
    void bookmarksRenewCache();
    void sameAsFullSort();
};

namespace {

const qint64 Minute = qint64(60) * 1000;
const qint64 Day = Minute * 60 * 24;

struct Sources
{
    QTemporaryDir dir;
    Storage storage{dir.path()};
    TabModel tabs{nullptr};
    BookmarkModel bookmarks{storage};
    HistoryModel history{storage};
    DownloadModel downloads{storage, dir.path()};
    QSettings file{dir.path() + QStringLiteral("/salama.conf"), QSettings::IniFormat};
    SearchEngines engines{file};
    SearchSettings search{file, engines};
    PrivacySettings privacy{file};
    OmnibarModel omnibar{&tabs, &bookmarks, &history, &downloads, &search, &privacy};
};

QVariant role(const OmnibarModel &model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

QStringList rows(const OmnibarModel &model)
{
    QStringList rows;
    for (int row = 0; row < model.rowCount(); ++row) {
        rows.append(role(model, row, roleId(OmnibarModel::Role::Kind)).toString() +
                    QStringLiteral(": ") +
                    role(model, row, roleId(OmnibarModel::Role::Title)).toString());
    }
    return rows;
}

int rowOfKind(const OmnibarModel &model, const QString &kind)
{
    for (int row = 0; row < model.rowCount(); ++row) {
        if (role(model, row, roleId(OmnibarModel::Role::Kind)).toString() == kind) {
            return row;
        }
    }
    return -1;
}

int countOfKind(const OmnibarModel &model, const QString &kind)
{
    int count = 0;
    for (int row = 0; row < model.rowCount(); ++row) {
        count += role(model, row, roleId(OmnibarModel::Role::Kind)).toString() == kind ? 1 : 0;
    }
    return count;
}

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

int startDownload(DownloadModel &model, int engineId, const QString &name, const QString &url)
{
    model.observe(model.topic(), QVariantMap{{QStringLiteral("msg"), QStringLiteral("dl-start")},
                                             {QStringLiteral("id"), static_cast<double>(engineId)},
                                             {QStringLiteral("displayName"), name},
                                             {QStringLiteral("sourceUrl"), url},
                                             {QStringLiteral("targetPath"), QString()}});
    return model.data(model.index(0, 0), roleId(DownloadModel::Role::DownloadId)).toInt();
}

void downloadMessage(DownloadModel &model, int engineId, const QVariantMap &extra)
{
    QVariantMap message = extra;
    message.insert(QStringLiteral("id"), static_cast<double>(engineId));
    model.observe(model.topic(), message);
}

// Requested rebuild runs on next event loop pass.
void settle()
{
    QTest::qWait(10);
}

// Oracle: ranking as it was before caching and partial sort (full parse, two stable sorts).
namespace Before {

using Salama::SearchWords;
using Salama::Tab;

QString orUrl(const QString &title, const QString &url)
{
    return title.isEmpty() ? url : title;
}

struct Page
{
    OmnibarRow row;
    int match = 3;
    double learnt = 0;
    int visits = 0;
    qint64 lastUsed = 0;
    qint64 frecency = 0;
};

int matchOf(const SearchWords &words, const OmnibarRow &row)
{
    if (!words.matches({row.title, row.url})) {
        return 3;
    }
    if (words.prefixes(row.host)) {
        return 0;
    }
    return words.prefixesAWordOf(row.title) || words.prefixesAWordOf(row.url) ? 1 : 2;
}

struct Search
{
    const SearchWords &words;
    QHash<QString, double> learnt;

    bool wants(const QString &title, const QString &url) const
    {
        return learnt.contains(url) || words.matches({title, url});
    }
};

struct Pages
{
    QList<Page> list;
    QHash<QString, int> byUrl;

    void add(const Page &page)
    {
        byUrl.insert(page.row.url, list.count());
        list.append(page);
    }
};

void addTabs(Pages &pages, const TabModel &model, const Search &search)
{
    QList<const Tab *> tabs;
    for (const Tab &tab : model.tabs()) {
        if (tab.id != model.activeTabId() && search.wants(tab.title, tab.url)) {
            tabs.append(&tab);
        }
    }
    std::stable_sort(tabs.begin(), tabs.end(), [](const Tab *one, const Tab *other) {
        return one->lastActive > other->lastActive;
    });
    for (const Tab *tab : tabs) {
        if (pages.byUrl.contains(tab->url)) {
            continue;
        }
        Page page;
        page.row.kind = OmnibarKind::Tab;
        page.row.id = tab->id;
        page.row.title = orUrl(tab->title, tab->url);
        page.row.url = tab->url;
        page.row.favicon = tab->favicon;
        page.row.groupId = tab->groupId;
        const int group = model.groupIndexOf(tab->groupId);
        page.row.groupName = group >= 0 ? model.groups().at(group).name : QString();
        page.row.groupTabCount = model.tabCountInGroup(tab->groupId);
        pages.add(page);
    }
}

void addBookmarks(Pages &pages, const BookmarkModel &model, const Search &search)
{
    for (const BookmarkModel::Bookmark &bookmark : model.bookmarks()) {
        if (pages.byUrl.contains(bookmark.url) || !search.wants(bookmark.title, bookmark.url)) {
            continue;
        }
        Page page;
        page.row.kind = OmnibarKind::Bookmark;
        page.row.id = bookmark.id;
        page.row.title = orUrl(bookmark.title, bookmark.url);
        page.row.url = bookmark.url;
        page.row.favicon = bookmark.favicon;
        page.lastUsed = bookmark.created;
        pages.add(page);
    }
}

using SiteIcons = QHash<QString, QString>;

void addSiteIcon(SiteIcons &icons, const QString &url, const QString &favicon)
{
    if (!favicon.isEmpty()) {
        const QString host = SearchSettings::displayAddress(url);
        if (!icons.contains(host)) {
            icons.insert(host, favicon);
        }
    }
}

void addHistory(Pages &pages, const HistoryModel &model, bool listed, const Search &search,
                SiteIcons &icons)
{
    for (const HistoryModel::Entry &entry : model.allEntries()) {
        addSiteIcon(icons, entry.url, entry.favicon);
        const auto known = pages.byUrl.constFind(entry.url);
        if (known != pages.byUrl.cend()) {
            Page &page = pages.list[known.value()];
            page.visits = entry.visitCount;
            page.lastUsed = std::max(page.lastUsed, entry.date.toMSecsSinceEpoch());
            if (page.row.favicon.isEmpty()) {
                page.row.favicon = entry.favicon;
            }
            continue;
        }
        if (!listed || !search.wants(entry.title, entry.url)) {
            continue;
        }
        Page page;
        page.row.kind = OmnibarKind::History;
        page.row.id = entry.id;
        page.row.title = orUrl(entry.title, entry.url);
        page.row.url = entry.url;
        page.row.favicon = entry.favicon;
        page.row.date = entry.date;
        page.visits = entry.visitCount;
        page.lastUsed = entry.date.toMSecsSinceEpoch();
        pages.add(page);
    }
}

void score(Page &page, const Search &search, bool bookmarked, const SiteIcons &icons, qint64 now)
{
    page.row.host = SearchSettings::displayAddress(page.row.url);
    page.row.markedTitle = search.words.marked(page.row.title);
    page.row.markedHost = search.words.marked(page.row.host);
    if (page.row.favicon.isEmpty()) {
        page.row.favicon = icons.value(page.row.host);
    }
    page.row.bookmarked = bookmarked;
    page.match = matchOf(search.words, page.row);
    page.learnt = search.learnt.value(page.row.url);
    if (page.row.kind == OmnibarKind::Tab && page.visits == 0) {
        page.lastUsed = now;
    }
    page.frecency = OmnibarModel::frecency(page.visits, bookmarked, page.lastUsed, now);
}

void rank(QList<Page> &pages)
{
    std::stable_sort(pages.begin(), pages.end(), [](const Page &one, const Page &other) {
        if (one.learnt != other.learnt) {
            return one.learnt > other.learnt;
        }
        return one.frecency > other.frecency;
    });
    int learntFirst = 0;
    while (learntFirst < pages.count() && learntFirst < OmnibarModel::MaxLearnt &&
           pages.at(learntFirst).learnt > 0) {
        ++learntFirst;
    }
    std::stable_sort(pages.begin() + learntFirst, pages.end(),
                     [](const Page &one, const Page &other) {
                         if (one.match != other.match) {
                             return one.match < other.match;
                         }
                         return one.frecency > other.frecency;
                     });
}

QList<OmnibarRow> pageRows(Sources &sources, const QString &query, int room)
{
    const SearchWords words(query);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const Search search{words, sources.history.inputRanks(query, now)};
    Pages pages;
    if (sources.search.omnibarTabs()) {
        addTabs(pages, sources.tabs, search);
    }
    if (sources.search.omnibarBookmarks()) {
        addBookmarks(pages, sources.bookmarks, search);
    }
    SiteIcons icons;
    for (const Tab &tab : sources.tabs.tabs()) {
        addSiteIcon(icons, tab.url, tab.favicon);
    }
    QSet<QString> bookmarked;
    for (const BookmarkModel::Bookmark &bookmark : sources.bookmarks.bookmarks()) {
        bookmarked.insert(bookmark.url);
        addSiteIcon(icons, bookmark.url, bookmark.favicon);
    }
    addHistory(pages, sources.history, sources.search.omnibarHistory(), search, icons);
    for (Page &page : pages.list) {
        score(page, search, bookmarked.contains(page.row.url), icons, now);
    }
    rank(pages.list);
    QList<OmnibarRow> rows;
    for (int i = 0; i < pages.list.count() && i < room; ++i) {
        rows.append(pages.list.at(i).row);
    }
    return rows;
}

} // namespace Before

QString kindName(OmnibarKind kind)
{
    switch (kind) {
    case OmnibarKind::Tab:
        return QStringLiteral("tab");
    case OmnibarKind::Bookmark:
        return QStringLiteral("bookmark");
    case OmnibarKind::History:
        return QStringLiteral("history");
    case OmnibarKind::Download:
        break;
    }
    return QStringLiteral("download");
}

QString described(const QString &kind, int tabId, const QVariantList &fields)
{
    QStringList parts{kind, QString::number(tabId)};
    for (const QVariant &field : fields) {
        parts.append(field.type() == QVariant::DateTime
                         ? QString::number(field.toDateTime().toMSecsSinceEpoch())
                         : field.toString());
    }
    return parts.join(QLatin1Char('|'));
}

// Page rows, every field; downloads left out.
QStringList pageFields(const OmnibarModel &model)
{
    using Role = OmnibarModel::Role;
    QStringList rows;
    for (int row = 0; row < model.rowCount(); ++row) {
        const QString kind = role(model, row, roleId(Role::Kind)).toString();
        if (kind == QStringLiteral("download")) {
            continue;
        }
        QVariantList fields;
        for (const Role field : {Role::Title, Role::Url, Role::Host, Role::MarkedTitle,
                                 Role::MarkedHost, Role::Favicon, Role::GroupId, Role::GroupName,
                                 Role::GroupTabCount, Role::Bookmarked, Role::Date}) {
            fields.append(role(model, row, roleId(field)));
        }
        rows.append(described(kind, role(model, row, roleId(Role::TabId)).toInt(), fields));
    }
    return rows;
}

QStringList pageFields(const QList<OmnibarRow> &rows)
{
    QStringList out;
    for (const OmnibarRow &row : rows) {
        out.append(
            described(kindName(row.kind), row.kind == OmnibarKind::Tab ? row.id : 0,
                      {row.title, row.url, row.host, row.markedTitle, row.markedHost, row.favicon,
                       row.groupId, row.groupName, row.groupTabCount, row.bookmarked, row.date}));
    }
    return out;
}

} // namespace

void tst_omnibarmodel::roles()
{
    Sources sources;
    const QHash<int, QByteArray> names = sources.omnibar.roleNames();
    // QML binds by name: none may move.
    const QList<QByteArray> expected{
        "kind",       "title",          "url",      "host",      "markedTitle",   "markedHost",
        "favicon",    "tabId",          "groupId",  "groupName", "groupTabCount", "bookmarked",
        "downloadId", "downloadStatus", "progress", "date",
    };
    QCOMPARE(names.count(), expected.count());
    for (const QByteArray &name : expected) {
        QVERIFY2(names.values().contains(name), name.constData());
    }
    QCOMPARE(names.value(roleId(OmnibarModel::Role::Kind)), QByteArray("kind"));
    QCOMPARE(names.value(roleId(OmnibarModel::Role::DownloadStatus)), QByteArray("downloadStatus"));

    QCOMPARE(sources.omnibar.rowCount(), 0);
    QCOMPARE(sources.omnibar.rowCount(sources.omnibar.index(0, 0)), 0);
    QVERIFY(!role(sources.omnibar, 0, roleId(OmnibarModel::Role::Kind)).isValid());
    QVERIFY(!role(sources.omnibar, -1, roleId(OmnibarModel::Role::Kind)).isValid());

    sources.bookmarks.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    sources.omnibar.setQuery(QStringLiteral("a"));
    QCOMPARE(sources.omnibar.count(), 1);
    QVERIFY(!role(sources.omnibar, 0, Qt::DisplayRole).isValid());
    QCOMPARE(sources.omnibar.rowCount(sources.omnibar.index(0, 0)), 0);
}

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

// Every word across fields, all sources; any missing = no match. Host prefix of first word
// first, then frecency: bookmark (2x page weight) before tab.
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

    const int tabRow = rowOfKind(omnibar, QStringLiteral("tab"));
    QCOMPARE(role(omnibar, tabRow, roleId(OmnibarModel::Role::Title)).toString(),
             QStringLiteral("https://www.row.example/tab"));
    QCOMPARE(role(omnibar, tabRow, roleId(OmnibarModel::Role::Url)).toString(),
             QStringLiteral("https://www.row.example/tab"));
    QCOMPARE(role(omnibar, tabRow, roleId(OmnibarModel::Role::Host)).toString(),
             QStringLiteral("row.example"));
    QCOMPARE(role(omnibar, tabRow, roleId(OmnibarModel::Role::Favicon)).toString(),
             QStringLiteral("https://row.example/tab.ico"));
    QCOMPARE(role(omnibar, tabRow, roleId(OmnibarModel::Role::TabId)).toInt(), tab);
    QCOMPARE(role(omnibar, tabRow, roleId(OmnibarModel::Role::GroupId)).toInt(),
             sources.tabs.defaultGroupId());
    QCOMPARE(role(omnibar, tabRow, roleId(OmnibarModel::Role::GroupTabCount)).toInt(), 2);
    QVERIFY(!role(omnibar, tabRow, roleId(OmnibarModel::Role::Bookmarked)).toBool());
    QCOMPARE(role(omnibar, tabRow, roleId(OmnibarModel::Role::DownloadId)).toInt(), 0);
    QVERIFY(!role(omnibar, tabRow, roleId(OmnibarModel::Role::Date)).toDateTime().isValid());

    const int bookmarkRow = rowOfKind(omnibar, QStringLiteral("bookmark"));
    QCOMPARE(role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::Title)).toString(),
             QStringLiteral("Bookmarked row"));
    QCOMPARE(role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::Url)).toString(),
             QStringLiteral("https://row.example/bookmark"));
    QCOMPARE(role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::Favicon)).toString(),
             QStringLiteral("https://row.example/b.ico"));
    QVERIFY(role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::Bookmarked)).toBool());
    QCOMPARE(role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::TabId)).toInt(), 0);
    QCOMPARE(role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::GroupId)).toInt(), 0);
    QVERIFY(role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::GroupName)).toString().isEmpty());
    QCOMPARE(role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::DownloadId)).toInt(), 0);
    QVERIFY(!role(omnibar, bookmarkRow, roleId(OmnibarModel::Role::Date)).toDateTime().isValid());

    const int historyRow = rowOfKind(omnibar, QStringLiteral("history"));
    QCOMPARE(role(omnibar, historyRow, roleId(OmnibarModel::Role::Title)).toString(),
             QStringLiteral("https://row.example/history"));
    QCOMPARE(role(omnibar, historyRow, roleId(OmnibarModel::Role::Host)).toString(),
             QStringLiteral("row.example"));
    QCOMPARE(role(omnibar, historyRow, roleId(OmnibarModel::Role::Favicon)).toString(),
             QStringLiteral("https://row.example/tab.ico"));
    QVERIFY(!role(omnibar, historyRow, roleId(OmnibarModel::Role::Bookmarked)).toBool());
    const QDateTime visited =
        role(omnibar, historyRow, roleId(OmnibarModel::Role::Date)).toDateTime();
    QVERIFY(visited.isValid());
    QVERIFY(qAbs(visited.msecsTo(QDateTime::currentDateTime()) - Day) < Minute);

    QCOMPARE(rowOfKind(omnibar, QStringLiteral("download")), 3);
    QCOMPARE(role(omnibar, 3, roleId(OmnibarModel::Role::Title)).toString(),
             QStringLiteral("row.pdf"));
    QCOMPARE(role(omnibar, 3, roleId(OmnibarModel::Role::Url)).toString(),
             QStringLiteral("https://files.row.example/row.pdf"));
    QCOMPARE(role(omnibar, 3, roleId(OmnibarModel::Role::Host)).toString(),
             QStringLiteral("files.row.example"));
    QCOMPARE(role(omnibar, 3, roleId(OmnibarModel::Role::DownloadId)).toInt(), download);
    QCOMPARE(sources.downloads.rowOf(download), 0);
    QCOMPARE(role(omnibar, 3, roleId(OmnibarModel::Role::DownloadStatus)).toInt(),
             static_cast<int>(DownloadModel::Running));
    QCOMPARE(role(omnibar, 3, roleId(OmnibarModel::Role::Progress)).toInt(), 40);
    QCOMPARE(role(omnibar, 3, roleId(OmnibarModel::Role::TabId)).toInt(), 0);
    QVERIFY(role(omnibar, 3, roleId(OmnibarModel::Role::Date)).toDateTime().isValid());
    QVERIFY(role(omnibar, 3, roleId(OmnibarModel::Role::Favicon)).toString().isEmpty());
    QCOMPARE(role(omnibar, 3, roleId(OmnibarModel::Role::MarkedTitle)).toString(),
             QStringLiteral("<b>row</b>.pdf"));

    startDownload(sources.downloads, 8, QString(), QStringLiteral("https://row.example/x"));
    omnibar.setQuery(QStringLiteral("row.example/x"));
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("download: https://row.example/x")});
}

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
    QCOMPARE(rows(omnibar),
             (QStringList{QStringLiteral("tab: Office mail"), QStringLiteral("tab: Home mail")}));
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::TabId)).toInt(), office);
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::GroupId)).toInt(), work);
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::GroupName)).toString(),
             QStringLiteral("Work"));
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::GroupTabCount)).toInt(), 2);
    QCOMPARE(role(omnibar, 1, roleId(OmnibarModel::Role::TabId)).toInt(), home);
    QCOMPARE(role(omnibar, 1, roleId(OmnibarModel::Role::GroupId)).toInt(), tabs.defaultGroupId());
    QVERIFY(role(omnibar, 1, roleId(OmnibarModel::Role::GroupName)).toString().isEmpty());
    QCOMPARE(role(omnibar, 1, roleId(OmnibarModel::Role::GroupTabCount)).toInt(), 1);

    tabs.renameGroup(work, QStringLiteral("Office"));
    QTRY_COMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::GroupName)).toString(),
                 QStringLiteral("Office"));
}

void tst_omnibarmodel::activeTabNotListed()
{
    Sources sources;
    TabModel &tabs = sources.tabs;
    const int first = tabs.newTab(QStringLiteral("https://one.example/"));
    const int second = tabs.newTab(QStringLiteral("https://two.example/"));

    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("example"));
    QCOMPARE(omnibar.count(), 1);
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::TabId)).toInt(), first);

    tabs.activateTabById(first);
    QTRY_COMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::TabId)).toInt(), second);
    QCOMPARE(omnibar.count(), 1);
}

// Rank: host prefix of first word, then title/url word prefix, then rest; within each
// frecency; ties keep source order.
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

    QVERIFY(addHistory(sources.storage, QStringLiteral("https://often.example/news"),
                       QStringLiteral("Often"), 50, Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://news.seldom.example/"),
                       QStringLiteral("Seldom"), 1, 100 * Day));
    // Raw rows bypass model; omnibar keeps table read: say what model's writes say.
    emit sources.history.entriesChanged();
    omnibar.setQuery(QStringLiteral("news example"));
    const QStringList found = rows(omnibar);
    QVERIFY(found.indexOf(QStringLiteral("history: Seldom")) >= 0);
    QVERIFY(found.indexOf(QStringLiteral("history: Seldom")) <
            found.indexOf(QStringLiteral("history: Often")));
    QVERIFY(found.indexOf(QStringLiteral("history: Often")) <
            found.indexOf(QStringLiteral("bookmark: Evening")));
}

// Firefox: score = day it decays to 1. Doubling = one half-life later; one day older = one less.
void tst_omnibarmodel::frecencyWeights()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 today = now / Day;
    // 50 -> 1 in 30 * log2(50) days = 169.x.
    QCOMPARE(OmnibarModel::frecency(1, false, now, now), today + 169);
    QCOMPARE(OmnibarModel::frecency(2, false, now, now), today + 199);
    QCOMPARE(OmnibarModel::frecency(1, true, now, now), today + 199);
    QCOMPARE(OmnibarModel::frecency(4, true, now, now), today + 259);
    QCOMPARE(OmnibarModel::frecency(1, false, now - Day, now), today + 168);
    QCOMPARE(OmnibarModel::frecency(2, false, now - 30 * Day, now), today + 169);
    QCOMPARE(OmnibarModel::frecency(1, false, now - 3650 * Day, now), today - 3650 + 169);
    QCOMPARE(OmnibarModel::frecency(0, false, now, now), today + 169);
    QCOMPARE(OmnibarModel::frecency(1, false, now + 5 * Day, now), today + 169);
}

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
    for (int i = 0; i < 6; ++i) {
        QCOMPARE(role(omnibar, i, roleId(OmnibarModel::Role::Title)).toString(),
                 QStringLiteral("Bookmark %1").arg(i));
    }
    QCOMPARE(role(omnibar, 6, roleId(OmnibarModel::Role::Title)).toString(),
             QStringLiteral("cap-6.pdf"));
    QCOMPARE(role(omnibar, 7, roleId(OmnibarModel::Role::Title)).toString(),
             QStringLiteral("cap-5.pdf"));

    omnibar.setQuery(QStringLiteral("cap 1"));
    QCOMPARE(omnibar.count(), 8);
    QCOMPARE(countOfKind(omnibar, QStringLiteral("download")), 1);
    QCOMPARE(rows(omnibar).last(), QStringLiteral("download: cap-1.pdf"));
}

// One row per page, any sources: tab before bookmark before history. Visits and
// bookmarked status count whichever row shows.
void tst_omnibarmodel::onePageOneRow()
{
    Sources sources;
    sources.tabs.newTab(QStringLiteral("https://shared.example/"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    sources.bookmarks.add(QStringLiteral("https://shared.example/"), QStringLiteral("Shared"));
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
                                QStringLiteral("bookmark: Front"),
                                QStringLiteral("bookmark: Mark"),
                                QStringLiteral("tab: https://shared.example/"),
                                QStringLiteral("history: Old page"),
                            }));
    QVERIFY(role(omnibar, 2, roleId(OmnibarModel::Role::Bookmarked)).toBool());
    QVERIFY(!role(omnibar, 3, roleId(OmnibarModel::Role::Bookmarked)).toBool());

    sources.search.setOmnibarTabs(false);
    QTRY_COMPARE(rows(omnibar).first(), QStringLiteral("bookmark: Shared"));
    QCOMPARE(omnibar.count(), 4);
    sources.search.setOmnibarBookmarks(false);
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("history")), 4);
    QCOMPARE(rows(omnibar).last(), QStringLiteral("history: Old page"));
    QVERIFY(role(omnibar, 0, roleId(OmnibarModel::Role::Bookmarked)).toBool());
}

// Learnt choices for typed text first, even without word match; max three, likeliest first.
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
    omnibar.setQuery(QStringLiteral("g"));
    QCOMPARE(rows(omnibar).first(), QStringLiteral("history: GitHub"));
    omnibar.setQuery(QStringLiteral("ghx"));
    QCOMPARE(omnibar.count(), 0);

    // Exact typed counts 2x prefix: "gi" once for Ghost beats "git" twice for GitHub.
    omnibar.learn(QStringLiteral("git"), QStringLiteral("https://github.com/"));
    omnibar.learn(QStringLiteral("git"), QStringLiteral("https://github.com/"));
    omnibar.learn(QStringLiteral("gi"), QStringLiteral("https://ghost.example/"));
    omnibar.setQuery(QStringLiteral("gi"));
    QCOMPARE(rows(omnibar),
             (QStringList{QStringLiteral("history: Ghost"), QStringLiteral("history: GitHub")}));

    for (int i = 0; i < 4; ++i) {
        const QString url = QStringLiteral("https://learnt-%1.example/").arg(i);
        QVERIFY(addHistory(sources.storage, url, QStringLiteral("Learnt %1").arg(i), 1, Day));
        for (int times = 0; times <= i; ++times) {
            omnibar.learn(QStringLiteral("zz"), url);
        }
    }
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://zz.example/"), QStringLiteral("Zz"),
                       1, 0));
    emit sources.history.entriesChanged();
    omnibar.setQuery(QStringLiteral("zz"));
    QCOMPARE(rows(omnibar), (QStringList{
                                QStringLiteral("history: Learnt 3"),
                                QStringLiteral("history: Learnt 2"),
                                QStringLiteral("history: Learnt 1"),
                                QStringLiteral("history: Zz"),
                                QStringLiteral("history: Learnt 0"),
                            }));
}

void tst_omnibarmodel::learningFollowsTheHistory()
{
    Sources sources;
    OmnibarModel &omnibar = sources.omnibar;
    sources.history.visit(QStringLiteral("https://kept.example/"), QStringLiteral("Kept"));
    sources.privacy.setRememberHistory(false);
    omnibar.learn(QStringLiteral("qq"), QStringLiteral("https://kept.example/"));
    omnibar.setQuery(QStringLiteral("qq"));
    QCOMPARE(omnibar.count(), 0);

    sources.privacy.setRememberHistory(true);
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

void tst_omnibarmodel::favicons()
{
    Sources sources;
    OmnibarModel &omnibar = sources.omnibar;
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://kept.example/a"),
                       QStringLiteral("Icon a"), 1, Day));
    sources.history.updateFavicon(QStringLiteral("https://kept.example/a"),
                                  QStringLiteral("a.ico"));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://kept.example/b"),
                       QStringLiteral("Icon b"), 1, Day));
    const int tab = sources.tabs.newTab(QStringLiteral("https://kept.example/b"));
    sources.tabs.newTab(QStringLiteral("https://front.example/"));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://www.kept.example/c"),
                       QStringLiteral("Icon c"), 1, Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://bare.example/"),
                       QStringLiteral("Icon d"), 1, Day));
    startDownload(sources.downloads, 1, QStringLiteral("icon.pdf"),
                  QStringLiteral("https://kept.example/icon.pdf"));

    const auto iconOf = [&omnibar](const QString &title) {
        for (int row = 0; row < omnibar.rowCount(); ++row) {
            if (role(omnibar, row, roleId(OmnibarModel::Role::Title)).toString() == title) {
                return role(omnibar, row, roleId(OmnibarModel::Role::Favicon)).toString();
            }
        }
        return QStringLiteral("(not listed)");
    };
    omnibar.setQuery(QStringLiteral("icon"));
    QCOMPARE(iconOf(QStringLiteral("Icon a")), QStringLiteral("a.ico"));
    QCOMPARE(iconOf(QStringLiteral("Icon c")), QStringLiteral("a.ico"));
    QVERIFY(iconOf(QStringLiteral("Icon d")).isEmpty());
    QVERIFY(iconOf(QStringLiteral("icon.pdf")).isEmpty());

    sources.tabs.updateTitle(tab, QStringLiteral("Icon b"));
    sources.tabs.updateFavicon(tab, QStringLiteral("b.ico"));
    omnibar.setQuery(QString());
    omnibar.setQuery(QStringLiteral("icon"));
    QCOMPARE(iconOf(QStringLiteral("Icon b")), QStringLiteral("b.ico"));
    QCOMPARE(iconOf(QStringLiteral("Icon c")), QStringLiteral("b.ico"));
    QCOMPARE(iconOf(QStringLiteral("Icon a")), QStringLiteral("a.ico"));
    sources.tabs.updateFavicon(tab, QString());
    sources.history.updateFavicon(QStringLiteral("https://kept.example/b"),
                                  QStringLiteral("b-kept.ico"));
    omnibar.setQuery(QString());
    omnibar.setQuery(QStringLiteral("icon"));
    QCOMPARE(iconOf(QStringLiteral("Icon b")), QStringLiteral("b-kept.ico"));
}

void tst_omnibarmodel::markedWords()
{
    Sources sources;
    OmnibarModel &omnibar = sources.omnibar;
    sources.bookmarks.add(QStringLiteral("https://forest.example/"),
                          QStringLiteral("Forest <walks> & more"));
    omnibar.setQuery(QStringLiteral("for walk"));
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::MarkedTitle)).toString(),
             QStringLiteral("<b>For</b>est &lt;<b>walk</b>s&gt; &amp; more"));
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::MarkedHost)).toString(),
             QStringLiteral("<b>for</b>est.example"));
    QSignalSpy changed(&omnibar, &OmnibarModel::dataChanged);
    omnibar.setQuery(QStringLiteral("forest"));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::MarkedTitle)).toString(),
             QStringLiteral("<b>Forest</b> &lt;walks&gt; &amp; more"));

    omnibar.setQuery(QString());
    omnibar.setBookmarksWhenEmpty(true);
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::MarkedTitle)).toString(),
             QStringLiteral("Forest &lt;walks&gt; &amp; more"));
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::MarkedHost)).toString(),
             QStringLiteral("forest.example"));
}

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
    SearchSettings &search = sources.search;
    omnibar.setQuery(QStringLiteral("off"));
    QCOMPARE(omnibar.count(), 4);

    search.setOmnibarTabs(false);
    QTRY_COMPARE(omnibar.count(), 3);
    QCOMPARE(countOfKind(omnibar, QStringLiteral("tab")), 0);

    search.setOmnibarBookmarks(false);
    QTRY_COMPARE(omnibar.count(), 2);
    QCOMPARE(countOfKind(omnibar, QStringLiteral("bookmark")), 0);

    search.setOmnibarHistory(false);
    QTRY_COMPARE(omnibar.count(), 1);
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::Kind)).toString(),
             QStringLiteral("download"));

    search.setOmnibarDownloads(false);
    QTRY_COMPARE(omnibar.count(), 0);

    search.setOmnibarHistory(true);
    QTRY_COMPARE(omnibar.count(), 1);
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("history: https://off-history.example/")});
}

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
    QCOMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::Title)).toString(),
             QStringLiteral("Bookmark 0"));
    QCOMPARE(role(omnibar, 11, roleId(OmnibarModel::Role::Title)).toString(),
             QStringLiteral("Bookmark 11"));
    QVERIFY(role(omnibar, 0, roleId(OmnibarModel::Role::Bookmarked)).toBool());

    omnibar.setQuery(QStringLiteral("tab"));
    QCOMPARE(rows(omnibar), QStringList{QStringLiteral("tab: https://tab.example/")});
    omnibar.setQuery(QString());
    QCOMPARE(omnibar.count(), 12);

    sources.bookmarks.remove(0);
    QTRY_COMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::Title)).toString(),
                 QStringLiteral("Bookmark 1"));
    QCOMPARE(omnibar.count(), 11);

    sources.search.setOmnibarBookmarks(false);
    QTRY_COMPARE(omnibar.count(), 0);
    sources.search.setOmnibarBookmarks(true);
    QTRY_COMPARE(omnibar.count(), 11);

    omnibar.setBookmarksWhenEmpty(false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(omnibar.count(), 0);
}

void tst_omnibarmodel::rebuildsOnSourceChange()
{
    Sources sources;
    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("fresh"));
    QCOMPARE(omnibar.count(), 0);
    QSignalSpy resetSpy(&omnibar, &OmnibarModel::modelReset);
    QSignalSpy resultsSpy(&omnibar, &OmnibarModel::resultsChanged);

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
    sources.search.setOmnibarTabs(false);
    settle();
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(changeSpy.count(), 0);
    QCOMPARE(resultsSpy.count(), 0);
    QCOMPARE(omnibar.count(), 0);

    // Query cleared while rebuild pending: pending one no-ops.
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

// Rebuilt same rows change in place, never reset: progress or favicon update must not
// destroy row under finger.
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
    QTRY_COMPARE(role(omnibar, 1, roleId(OmnibarModel::Role::Progress)).toInt(), 50);
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(changeSpy.count(), 1);
    QCOMPARE(changeSpy.last().at(0).toModelIndex().row(), 1);
    QCOMPARE(changeSpy.last().at(1).toModelIndex().row(), 1);
    QCOMPARE(resultsSpy.count(), 0);

    downloadMessage(sources.downloads, 3,
                    {{QStringLiteral("msg"), QStringLiteral("dl-done")},
                     {QStringLiteral("targetPath"), QStringLiteral("/tmp/report.pdf")}});
    QTRY_COMPARE(role(omnibar, 1, roleId(OmnibarModel::Role::DownloadStatus)).toInt(),
                 static_cast<int>(DownloadModel::Done));
    QCOMPARE(role(omnibar, 1, roleId(OmnibarModel::Role::Progress)).toInt(), 100);
    QCOMPARE(resetSpy.count(), 0);

    sources.tabs.updateFavicon(tab, QStringLiteral("https://report.example/favicon.ico"));
    QTRY_COMPARE(role(omnibar, 0, roleId(OmnibarModel::Role::Favicon)).toString(),
                 QStringLiteral("https://report.example/favicon.ico"));
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(changeSpy.last().at(0).toModelIndex().row(), 0);

    const int changes = changeSpy.count();
    sources.tabs.updateThumbnail(tab, QStringLiteral("/nowhere/tab.png"));
    settle();
    QCOMPARE(changeSpy.count(), changes);
    QCOMPARE(resetSpy.count(), 0);

    sources.bookmarks.add(QStringLiteral("https://report-bookmark.example/"), QString());
    QTRY_COMPARE(resetSpy.count(), 1);
    QCOMPARE(omnibar.count(), 3);
    QCOMPARE(resultsSpy.count(), 1);
}

// History moves and inserts rows on visit, no reset: open pane follows both.
void tst_omnibarmodel::openPaneSeesVisits()
{
    Sources sources;
    sources.history.visit(QStringLiteral("https://known.example/a"), QStringLiteral("Known a"));
    sources.history.visit(QStringLiteral("https://known.example/b"), QStringLiteral("Known b"));
    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("known"));
    QCOMPARE(rows(omnibar),
             (QStringList{QStringLiteral("history: Known b"), QStringLiteral("history: Known a")}));

    sources.history.visit(QStringLiteral("https://known.example/a"), QStringLiteral("Known again"));
    QTRY_COMPARE(rows(omnibar), (QStringList{QStringLiteral("history: Known again"),
                                             QStringLiteral("history: Known b")}));
    sources.history.visit(QStringLiteral("https://known.example/c"));
    QTRY_COMPARE(countOfKind(omnibar, QStringLiteral("history")), 3);
}

// Keystrokes with history unchanged read table once: row written behind model's back stays
// unseen until model says history changed.
void tst_omnibarmodel::typingReadsHistoryOnce()
{
    Sources sources;
    sources.history.visit(QStringLiteral("https://pages.example/one"), QStringLiteral("Page one"));
    sources.history.visit(QStringLiteral("https://pages.example/two"), QStringLiteral("Page two"));
    OmnibarModel &omnibar = sources.omnibar;
    omnibar.setQuery(QStringLiteral("p"));
    QCOMPARE(countOfKind(omnibar, QStringLiteral("history")), 2);

    QSqlQuery wipe(sources.storage.database());
    QVERIFY(wipe.exec(QStringLiteral("DELETE FROM browser_history")));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://pages.example/three"),
                       QStringLiteral("Page three"), 1, 0));
    omnibar.setQuery(QStringLiteral("pa"));
    omnibar.setQuery(QStringLiteral("pag"));
    QCOMPARE(rows(omnibar), (QStringList{QStringLiteral("history: Page two"),
                                         QStringLiteral("history: Page one")}));

    sources.history.visit(QStringLiteral("https://pages.example/four"),
                          QStringLiteral("Page four"));
    QTRY_COMPARE(rows(omnibar), (QStringList{QStringLiteral("history: Page four"),
                                             QStringLiteral("history: Page three")}));
}

// Bookmarked mark and site icons come from kept bookmark parse: bookmark change renews it.
void tst_omnibarmodel::bookmarksRenewCache()
{
    Sources sources;
    sources.history.visit(QStringLiteral("https://cache.example/a"), QStringLiteral("Cache a"));
    sources.history.visit(QStringLiteral("https://cache.example/b"), QStringLiteral("Cache b"));
    OmnibarModel &omnibar = sources.omnibar;
    sources.search.setOmnibarBookmarks(false);
    omnibar.setQuery(QStringLiteral("cache"));
    QCOMPARE(countOfKind(omnibar, QStringLiteral("history")), 2);
    QVERIFY(!role(omnibar, 0, roleId(OmnibarModel::Role::Bookmarked)).toBool());

    sources.bookmarks.add(QStringLiteral("https://cache.example/b"), QStringLiteral("Mark"),
                          QStringLiteral("mark.ico"));
    QTRY_VERIFY(role(omnibar, 0, roleId(OmnibarModel::Role::Bookmarked)).toBool());
    QCOMPARE(role(omnibar, 1, roleId(OmnibarModel::Role::Favicon)).toString(),
             QStringLiteral("mark.ico"));
}

// Cached hosts and partial sort give rows, order and fields of full parse and two stable sorts:
// mixed sources, shared pages, learnt inputs past MaxLearnt, many frecency ties.
void tst_omnibarmodel::sameAsFullSort()
{
    Sources sources;
    const auto historyUrl = [](int i) {
        switch (i % 4) {
        case 0:
            return QStringLiteral("https://www.site%1.example/news/%2").arg(i % 37).arg(i);
        case 1:
            return QStringLiteral("https://site%1.example/?q=helsinki+%2").arg(i % 37).arg(i);
        case 2:
            return QStringLiteral("http://h%1.test/path%2").arg(i % 11).arg(i);
        default:
            return QStringLiteral("https://tampere.example/%1").arg(i);
        }
    };
    {
        QSqlQuery query(sources.storage.database());
        QVERIFY(query.exec(QStringLiteral("BEGIN")));
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (int i = 0; i < 600; ++i) {
            QSqlQuery insert(sources.storage.database());
            insert.prepare(QStringLiteral("INSERT INTO browser_history (url, title, "
                                          "visited_count, date, favicon) VALUES (?, ?, ?, ?, ?)"));
            insert.addBindValue(historyUrl(i));
            QString title = QStringLiteral("");
            if (i % 6 != 0) {
                title = i % 2 != 0 ? QStringLiteral("Title %1 news").arg(i % 50)
                                   : QStringLiteral("Helsinki <%1> & co").arg(i % 13);
            }
            insert.addBindValue(title);
            insert.addBindValue(1 + i % 4);
            insert.addBindValue(now - (i % 9) * Day - (i % 2) * Minute);
            insert.addBindValue(i % 5 == 0 ? QStringLiteral("fav%1.ico").arg(i % 37)
                                           : QStringLiteral(""));
            QVERIFY(insert.exec());
        }
        QVERIFY(query.exec(QStringLiteral("COMMIT")));
    }
    TabModel &tabs = sources.tabs;
    for (int i = 0; i < 6; ++i) {
        const int tab = tabs.newTab(i % 2 != 0 ? historyUrl(i * 8)
                                               : QStringLiteral("https://tab%1.example/h").arg(i));
        tabs.updateTitle(tab, i % 3 != 0 ? QStringLiteral("Tab %1 news").arg(i) : QString());
        if (i % 2 == 0) {
            tabs.updateFavicon(tab, QStringLiteral("tab%1.ico").arg(i));
        }
    }
    tabs.addGroup(QStringLiteral("Work"));
    for (int i = 0; i < 4; ++i) {
        tabs.newTab(historyUrl(4 * i + 1));
    }
    tabs.newTab(QStringLiteral("https://front.example/"));
    for (int i = 0; i < 30; ++i) {
        QString url = QStringLiteral("https://mark%1.example/").arg(i);
        if (i % 3 != 2) {
            url = historyUrl(i % 3 == 0 ? 4 * i + 3 : 8 * i);
        }
        sources.bookmarks.add(url, i % 4 != 0 ? QStringLiteral("Mark %1 news").arg(i) : QString(),
                              i % 5 == 0 ? QStringLiteral("mark%1.ico").arg(i) : QString());
    }
    HistoryModel &history = sources.history;
    for (int i = 0; i < 5; ++i) {
        for (int times = 0; times <= i % 3; ++times) {
            history.recordInput(QStringLiteral("h"), historyUrl(i * 5));
        }
        history.recordInput(QStringLiteral("t"), historyUrl(i * 7 + 2));
        history.recordInput(QStringLiteral("zz"), historyUrl(i * 3));
    }
    history.recordInput(QStringLiteral("he"), historyUrl(8));
    history.recordInput(QStringLiteral("zz"), QStringLiteral("https://mark2.example/"));
    // Learnt past cap, same match and frecency: more learnt, later in source, still first.
    for (int i = 0; i < 5; ++i) {
        const QString url = QStringLiteral("https://lt%1.example/").arg(i);
        QVERIFY(addHistory(sources.storage, url, QString(), 1, Day));
        for (int times = i; times < 5; ++times) {
            history.recordInput(QStringLiteral("lt"), url);
        }
    }
    // Learnt alike: higher frecency, later in source, first.
    QVERIFY(
        addHistory(sources.storage, QStringLiteral("https://lu1.example/"), QString(), 10, Day));
    QVERIFY(addHistory(sources.storage, QStringLiteral("https://lu0.example/"), QString(), 1, 0));
    // Same use time: equal rank, no decay gap.
    const qint64 used = QDateTime::currentMSecsSinceEpoch() - Day;
    for (const QString &url :
         {QStringLiteral("https://lu0.example/"), QStringLiteral("https://lu1.example/")}) {
        QSqlQuery learn(sources.storage.database());
        learn.prepare(QStringLiteral("INSERT INTO input_history (input, url, use_count, used) "
                                     "VALUES ('lu', ?, 1, ?)"));
        learn.addBindValue(url);
        learn.addBindValue(used);
        QVERIFY(learn.exec());
    }
    startDownload(sources.downloads, 1, QStringLiteral("e-report.pdf"),
                  QStringLiteral("https://files.example/e.pdf"));
    startDownload(sources.downloads, 2, QStringLiteral("news.pdf"),
                  QStringLiteral("https://files.example/news.pdf"));

    OmnibarModel &omnibar = sources.omnibar;
    const QStringList queries{
        QStringLiteral("h"),       QStringLiteral("he"),
        QStringLiteral("hel"),     QStringLiteral("helsinki news"),
        QStringLiteral("t"),       QStringLiteral("ti"),
        QStringLiteral("e"),       QStringLiteral("ex"),
        QStringLiteral("example"), QStringLiteral("news"),
        QStringLiteral("site1"),   QStringLiteral("site 1"),
        QStringLiteral("www"),     QStringLiteral("tampere"),
        QStringLiteral("zz"),      QStringLiteral("q"),
        QStringLiteral("1"),       QStringLiteral("fav"),
        QStringLiteral("http"),    QStringLiteral("<1>"),
        QStringLiteral("&"),       QStringLiteral("nope"),
        QStringLiteral("lt"),      QStringLiteral("lu"),
    };
    const auto compareAll = [&]() {
        for (const QString &query : queries) {
            omnibar.setQuery(QString());
            omnibar.setQuery(query);
            const int room =
                OmnibarModel::MaxRows - countOfKind(omnibar, QStringLiteral("download"));
            QCOMPARE(pageFields(omnibar), pageFields(Before::pageRows(sources, query, room)));
        }
    };
    compareAll();
    if (QTest::currentTestFailed()) {
        return;
    }
    omnibar.setQuery(QStringLiteral("lt"));
    QCOMPARE(rows(omnibar), (QStringList{
                                QStringLiteral("history: https://lt0.example/"),
                                QStringLiteral("history: https://lt1.example/"),
                                QStringLiteral("history: https://lt2.example/"),
                                QStringLiteral("history: https://lt3.example/"),
                                QStringLiteral("history: https://lt4.example/"),
                            }));
    omnibar.setQuery(QStringLiteral("lu"));
    QCOMPARE(rows(omnibar), (QStringList{QStringLiteral("history: https://lu1.example/"),
                                         QStringLiteral("history: https://lu0.example/")}));
    omnibar.setQuery(QStringLiteral("h"));
    QCOMPARE(omnibar.count(), OmnibarModel::MaxRows);
    sources.search.setOmnibarTabs(false);
    compareAll();
    if (QTest::currentTestFailed()) {
        return;
    }
    sources.search.setOmnibarHistory(false);
    compareAll();
}

QTEST_GUILESS_MAIN(tst_omnibarmodel)
#include "tst_omnibarmodel.moc"
