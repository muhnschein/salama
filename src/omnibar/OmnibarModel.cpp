// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "OmnibarModel.h"

#include "bookmarks/BookmarkModel.h"
#include "downloads/DownloadModel.h"
#include "history/HistoryModel.h"
#include "search/SearchWords.h"
#include "settings/Settings.h"
#include "tabs/TabModel.h"

#include <QHash>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace Salama {

namespace {

const qint64 Day = qint64(24) * 60 * 60 * 1000;
const double HalfLifeDays = 30;

QString orUrl(const QString &title, const QString &url)
{
    return title.isEmpty() ? url : title;
}

// A page on its way to being a row: the row, and what it is ranked by.
struct Page
{
    OmnibarRow row;
    // How well the words match: 0 when the host begins with the first word, 1 when a
    // word of the title or the address does, 2 when they are only somewhere in it, and
    // 3 when they are not in it at all and it is here for having been chosen after them.
    int match = 3;
    // How strongly what is typed leads here from choices before (HistoryModel::inputRanks).
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

// What is typed, and what it is known to lead to (HistoryModel::inputRanks).
struct Search
{
    const SearchWords &words;
    QHash<QString, double> learnt;

    // A candidate is listed when it holds every word, or when what is typed has led to
    // it before.
    bool wants(const QString &title, const QString &url) const
    {
        return learnt.contains(url) || words.matches({title, url});
    }
};

// The pages gathered so far, and where each address is among them.
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

// Every group's tabs but the one in front, most recently in front first, so of two tabs
// on one page the nearer to hand is it.
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

// In the bookmarks' own order, but for those open in a tab listed.
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

// A site's icon, as any page of it has shown one, by host: the first given is kept.
using SiteIcons = QHash<QString, QString>;

void addSiteIcon(SiteIcons &icons, const QString &url, const QString &favicon)
{
    if (!favicon.isEmpty()) {
        const QString host = Settings::displayAddress(url);
        if (!icons.contains(host)) {
            icons.insert(host, favicon);
        }
    }
}

// The visits of the pages already gathered, and -- when the history is a source -- the
// pages of it that are not, newest first. The icons the history kept go to the pages
// that have none, and to their sites.
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

// What a page shows and is ranked by. Without an icon of its own, its site's. A tab the
// history does not know is open now, which is a visit now.
void score(Page &page, const Search &search, bool bookmarked, const SiteIcons &icons, qint64 now)
{
    page.row.host = Settings::displayAddress(page.row.url);
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

// The pages chosen before, the likeliest first, no more than OmnibarModel::MaxLearnt;
// then the rest by how well they match and how often and how lately they were used.
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

} // namespace

OmnibarModel::OmnibarModel(TabModel *tabs, BookmarkModel *bookmarks, HistoryModel *history,
                           DownloadModel *downloads, Settings *settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_tabs(tabs)
    , m_bookmarks(bookmarks)
    , m_history(history)
    , m_downloads(downloads)
    , m_settings(settings)
{
    m_refresh.setSingleShot(true);
    m_refresh.setInterval(0);
    connect(&m_refresh, &QTimer::timeout, this, &OmnibarModel::rebuild);

    // A tab opened, closed, moved, renamed, loaded or brought to the front -- the one in
    // front is left out -- and a group renamed.
    connect(m_tabs, &TabModel::countChanged, this, &OmnibarModel::sourceChanged);
    connect(m_tabs, &TabModel::dataChanged, this, &OmnibarModel::sourceChanged);
    connect(m_tabs, &TabModel::rowsMoved, this, &OmnibarModel::sourceChanged);
    connect(m_tabs, &TabModel::groupsChanged, this, &OmnibarModel::sourceChanged);
    connect(m_tabs, &TabModel::activeTabChanged, this, &OmnibarModel::sourceChanged);
    connect(m_bookmarks, &BookmarkModel::revisionChanged, this, &OmnibarModel::sourceChanged);
    // A download started, gone, or on its way: count alone would miss the oldest going
    // as a new one comes.
    connect(m_downloads, &DownloadModel::rowsInserted, this, &OmnibarModel::sourceChanged);
    connect(m_downloads, &DownloadModel::rowsRemoved, this, &OmnibarModel::sourceChanged);
    connect(m_downloads, &DownloadModel::dataChanged, this, &OmnibarModel::sourceChanged);
    // The history model reads itself again on every visit, and changes a row in place
    // for a title and takes one out for a removal.
    connect(m_history, &HistoryModel::modelReset, this, &OmnibarModel::sourceChanged);
    connect(m_history, &HistoryModel::rowsRemoved, this, &OmnibarModel::sourceChanged);
    connect(m_history, &HistoryModel::dataChanged, this, &OmnibarModel::sourceChanged);
    connect(m_settings, &Settings::omnibarTabsChanged, this, &OmnibarModel::sourceChanged);
    connect(m_settings, &Settings::omnibarBookmarksChanged, this, &OmnibarModel::sourceChanged);
    connect(m_settings, &Settings::omnibarHistoryChanged, this, &OmnibarModel::sourceChanged);
    connect(m_settings, &Settings::omnibarDownloadsChanged, this, &OmnibarModel::sourceChanged);
}

int OmnibarModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_rows.count();
}

QVariant OmnibarModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.count()) {
        return {};
    }
    const OmnibarRow &row = m_rows.at(index.row());
    switch (role) {
    case KindRole:
        return kindName(row.kind);
    case TitleRole:
        return row.title;
    case UrlRole:
        return row.url;
    case HostRole:
        return row.host;
    case MarkedTitleRole:
        return row.markedTitle;
    case MarkedHostRole:
        return row.markedHost;
    case FaviconRole:
        return row.favicon;
    case TabIdRole:
        return row.kind == OmnibarKind::Tab ? row.id : 0;
    case GroupIdRole:
        return row.groupId;
    case GroupNameRole:
        return row.groupName;
    case GroupTabCountRole:
        return row.groupTabCount;
    case BookmarkedRole:
        return row.bookmarked;
    case DownloadIdRole:
        return row.kind == OmnibarKind::Download ? row.id : 0;
    case DownloadStatusRole:
        return row.downloadStatus;
    case ProgressRole:
        return row.progress;
    case DateRole:
        return row.date;
    default:
        return {};
    }
}

QHash<int, QByteArray> OmnibarModel::roleNames() const
{
    return {
        {KindRole, QByteArrayLiteral("kind")},
        {TitleRole, QByteArrayLiteral("title")},
        {UrlRole, QByteArrayLiteral("url")},
        {HostRole, QByteArrayLiteral("host")},
        {MarkedTitleRole, QByteArrayLiteral("markedTitle")},
        {MarkedHostRole, QByteArrayLiteral("markedHost")},
        {FaviconRole, QByteArrayLiteral("favicon")},
        {TabIdRole, QByteArrayLiteral("tabId")},
        {GroupIdRole, QByteArrayLiteral("groupId")},
        {GroupNameRole, QByteArrayLiteral("groupName")},
        {GroupTabCountRole, QByteArrayLiteral("groupTabCount")},
        {BookmarkedRole, QByteArrayLiteral("bookmarked")},
        {DownloadIdRole, QByteArrayLiteral("downloadId")},
        {DownloadStatusRole, QByteArrayLiteral("downloadStatus")},
        {ProgressRole, QByteArrayLiteral("progress")},
        {DateRole, QByteArrayLiteral("date")},
    };
}

QString OmnibarModel::query() const
{
    return m_query;
}

void OmnibarModel::setQuery(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (trimmed == m_query) {
        return;
    }
    m_query = trimmed;
    emit queryChanged();
    // At once, not on the next turn: the pane reads the rows as soon as it has set
    // the query, and what was waiting for the event loop is built with them.
    m_refresh.stop();
    rebuild();
}

bool OmnibarModel::bookmarksWhenEmpty() const
{
    return m_bookmarksWhenEmpty;
}

void OmnibarModel::setBookmarksWhenEmpty(bool on)
{
    if (on == m_bookmarksWhenEmpty) {
        return;
    }
    m_bookmarksWhenEmpty = on;
    emit bookmarksWhenEmptyChanged();
    m_refresh.stop();
    rebuild();
}

int OmnibarModel::count() const
{
    return m_rows.count();
}

void OmnibarModel::learn(const QString &typed, const QString &url) const
{
    if (m_settings->rememberHistory()) {
        m_history->recordInput(typed, url);
    }
}

qint64 OmnibarModel::frecency(int visitCount, bool bookmarked, qint64 lastUsed, qint64 now)
{
    const double lambda = std::log(2.0) / HalfLifeDays;
    const qint64 today = now / Day;
    // A use stamped ahead of the clock -- a clock set back -- is one today.
    const qint64 age = today - std::min(lastUsed, now) / Day;
    const double score =
        (bookmarked ? 100 : 50) * std::exp(-lambda * double(age)) * std::max(visitCount, 1);
    return today + qint64(std::floor(std::log(score) / lambda));
}

// Anything to show: something typed, or the bookmarks asked for with nothing typed.
// While there is not, the sources change unheard, and the empty list stays empty.
bool OmnibarModel::active() const
{
    return !m_query.isEmpty() || m_bookmarksWhenEmpty;
}

void OmnibarModel::sourceChanged()
{
    if (active()) {
        m_refresh.start();
    }
}

void OmnibarModel::rebuild()
{
    const QList<OmnibarRow> rows = collect();
    const bool sameRows = std::equal(rows.cbegin(), rows.cend(), m_rows.cbegin(), m_rows.cend(),
                                     [](const OmnibarRow &one, const OmnibarRow &other) {
                                         return one.kind == other.kind && one.id == other.id;
                                     });
    if (sameRows) {
        int first = -1;
        int last = -1;
        for (int i = 0; i < rows.count(); ++i) {
            if (rows.at(i) != m_rows.at(i)) {
                first = first < 0 ? i : first;
                last = i;
            }
        }
        m_rows = rows;
        if (first >= 0) {
            emit dataChanged(index(first, 0), index(last, 0));
        }
        return;
    }
    beginResetModel();
    m_rows = rows;
    endResetModel();
    emit resultsChanged();
}

QList<OmnibarRow> OmnibarModel::collect() const
{
    if (!active()) {
        return {};
    }
    const SearchWords words(m_query);
    if (words.isEmpty()) {
        return emptyRows();
    }
    const QList<OmnibarRow> downloads = downloadRows(words);
    return pageRows(words, MaxRows - downloads.count()) + downloads;
}

// Every bookmark, in the bookmarks' own order.
QList<OmnibarRow> OmnibarModel::emptyRows() const
{
    QList<OmnibarRow> rows;
    if (!m_settings->omnibarBookmarks()) {
        return rows;
    }
    for (const BookmarkModel::Bookmark &bookmark : m_bookmarks->bookmarks()) {
        OmnibarRow row;
        row.kind = OmnibarKind::Bookmark;
        row.id = bookmark.id;
        row.title = orUrl(bookmark.title, bookmark.url);
        row.url = bookmark.url;
        row.host = Settings::displayAddress(bookmark.url);
        row.markedTitle = row.title.toHtmlEscaped();
        row.markedHost = row.host.toHtmlEscaped();
        row.favicon = bookmark.favicon;
        row.bookmarked = true;
        rows.append(row);
    }
    return rows;
}

// The pages, one row an address, gathered from the open tabs, the bookmarks and the
// history in that order -- the first to hold an address decides what its row does --
// and ranked as the class says. A page's visits are counted whether or not the history
// is a source: a tab visited often ranks as often visited.
QList<OmnibarRow> OmnibarModel::pageRows(const SearchWords &words, int room) const
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const Search search{words, m_history->inputRanks(m_query, now)};
    Pages pages;
    if (m_settings->omnibarTabs()) {
        addTabs(pages, *m_tabs, search);
    }
    if (m_settings->omnibarBookmarks()) {
        addBookmarks(pages, *m_bookmarks, search);
    }
    // The icons of the sites open in tabs, then bookmarked, then in the history.
    SiteIcons icons;
    for (const Tab &tab : m_tabs->tabs()) {
        addSiteIcon(icons, tab.url, tab.favicon);
    }
    QSet<QString> bookmarked;
    for (const BookmarkModel::Bookmark &bookmark : m_bookmarks->bookmarks()) {
        bookmarked.insert(bookmark.url);
        addSiteIcon(icons, bookmark.url, bookmark.favicon);
    }
    addHistory(pages, *m_history, m_settings->omnibarHistory(), search, icons);

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

// Newest first, as the downloads list has them. The host is where the file came from.
QList<OmnibarRow> OmnibarModel::downloadRows(const SearchWords &words) const
{
    QList<OmnibarRow> rows;
    if (!m_settings->omnibarDownloads()) {
        return rows;
    }
    for (const DownloadModel::Download &download : m_downloads->downloads()) {
        if (rows.count() == MaxDownloads) {
            break;
        }
        if (!words.matches({download.name, download.url})) {
            continue;
        }
        OmnibarRow row;
        row.kind = OmnibarKind::Download;
        row.id = download.id;
        row.title = orUrl(download.name, download.url);
        row.url = download.url;
        row.host = Settings::displayAddress(download.url);
        row.markedTitle = words.marked(row.title);
        row.markedHost = words.marked(row.host);
        row.downloadStatus = static_cast<int>(download.status);
        row.progress = download.progress;
        row.date = QDateTime::fromMSecsSinceEpoch(download.started);
        rows.append(row);
    }
    return rows;
}

} // namespace Salama
