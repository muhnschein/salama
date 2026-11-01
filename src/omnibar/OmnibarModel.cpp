// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "OmnibarModel.h"

#include "bookmarks/BookmarkModel.h"
#include "downloads/DownloadModel.h"
#include "history/HistoryModel.h"
#include "search/SearchWords.h"
#include "settings/PrivacySettings.h"
#include "settings/SearchSettings.h"
#include "tabs/TabModel.h"

#include <QHash>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace Salama {

namespace {

const qint64 Day = qint64(24) * 60 * 60 * 1000;
const double HalfLifeDays = 30;

QString orUrl(const QString &title, const QString &url)
{
    return title.isEmpty() ? url : title;
}

struct Page
{
    OmnibarRow row;
    // 0 host prefix, 1 word prefix, 2 anywhere, 3 none (learnt only).
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

using SiteIcons = QHash<QString, QString>;

// URL parse once per source change, not per keystroke.
struct Sites
{
    bool valid = false;
    // url -> displayAddress.
    QHash<QString, QString> hosts;
    // host -> first favicon in source order.
    SiteIcons icons;

    void add(const QString &url, const QString &favicon)
    {
        auto host = hosts.constFind(url);
        if (host == hosts.cend()) {
            host = hosts.insert(url, SearchSettings::displayAddress(url));
        }
        if (!favicon.isEmpty() && !icons.contains(host.value())) {
            icons.insert(host.value(), favicon);
        }
    }

    QString hostOf(const QString &url) const
    {
        const auto host = hosts.constFind(url);
        return host != hosts.cend() ? host.value() : SearchSettings::displayAddress(url);
    }
};

struct HistoryPage
{
    HistoryModel::Entry entry;
    QString host;
    qint64 lastUsed = 0;
};

// Whole table, read once per history change.
struct HistoryPages
{
    bool valid = false;
    QVector<HistoryPage> pages;
    SiteIcons icons;
};

const Sites &tabSites(Sites &sites, const TabModel &model)
{
    if (!sites.valid) {
        for (const Tab &tab : model.tabs()) {
            sites.add(tab.url, tab.favicon);
        }
        sites.valid = true;
    }
    return sites;
}

const Sites &bookmarkSites(Sites &sites, const BookmarkModel &model)
{
    if (!sites.valid) {
        for (const BookmarkModel::Bookmark &bookmark : model.bookmarks()) {
            sites.add(bookmark.url, bookmark.favicon);
        }
        sites.valid = true;
    }
    return sites;
}

const HistoryPages &historyPages(HistoryPages &history, const HistoryModel &model)
{
    if (!history.valid) {
        const QList<HistoryModel::Entry> entries = model.allEntries();
        history.pages.reserve(entries.count());
        for (const HistoryModel::Entry &entry : entries) {
            const HistoryPage page{entry, SearchSettings::displayAddress(entry.url),
                                   entry.date.toMSecsSinceEpoch()};
            if (!entry.favicon.isEmpty() && !history.icons.contains(page.host)) {
                history.icons.insert(page.host, entry.favicon);
            }
            history.pages.append(page);
        }
        history.valid = true;
    }
    return history;
}

QString siteIcon(const QString &host, std::initializer_list<const SiteIcons *> sources)
{
    for (const SiteIcons *icons : sources) {
        const auto icon = icons->constFind(host);
        if (icon != icons->cend()) {
            return icon.value();
        }
    }
    return {};
}

// Most recent first: of two tabs on one page, nearer wins.
void addTabs(Pages &pages, const TabModel &model, const Search &search, const Sites &sites)
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
        page.row.host = sites.hostOf(tab->url);
        page.row.favicon = tab->favicon;
        page.row.groupId = tab->groupId;
        const int group = model.groupIndexOf(tab->groupId);
        page.row.groupName = group >= 0 ? model.groups().at(group).name : QString();
        page.row.groupTabCount = model.tabCountInGroup(tab->groupId);
        pages.add(page);
    }
}

void addBookmarks(Pages &pages, const BookmarkModel &model, const Search &search,
                  const Sites &sites)
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
        page.row.host = sites.hostOf(bookmark.url);
        page.row.favicon = bookmark.favicon;
        page.lastUsed = bookmark.created;
        pages.add(page);
    }
}

// History icon fills page lacking one.
void addHistory(Pages &pages, const HistoryPages &history, bool listed, const Search &search)
{
    for (const HistoryPage &known : history.pages) {
        const HistoryModel::Entry &entry = known.entry;
        const auto shown = pages.byUrl.constFind(entry.url);
        if (shown != pages.byUrl.cend()) {
            Page &page = pages.list[shown.value()];
            page.visits = entry.visitCount;
            page.lastUsed = std::max(page.lastUsed, known.lastUsed);
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
        page.row.host = known.host;
        page.row.favicon = entry.favicon;
        page.row.date = entry.date;
        page.visits = entry.visitCount;
        page.lastUsed = known.lastUsed;
        pages.add(page);
    }
}

// Tab unknown to history counts as visit now.
void score(Page &page, const Search &search, bool bookmarked, qint64 now)
{
    page.row.bookmarked = bookmarked;
    page.match = matchOf(search.words, page.row);
    page.learnt = search.learnt.value(page.row.url);
    if (page.row.kind == OmnibarKind::Tab && page.visits == 0) {
        page.lastUsed = now;
    }
    page.frecency = OmnibarModel::frecency(page.visits, bookmarked, page.lastUsed, now);
}

// Indices of first `room` pages. Order = stable sort by (learnt, frecency), top MaxLearnt
// learnt kept, then rest stable-sorted by (match, frecency). Ties fall back to that first
// order (learnt, then source index), so partial_sort gives same rows, sorting only shown.
QVector<int> rank(const QList<Page> &pages, int room)
{
    QVector<int> learnt;
    QVector<int> rest;
    for (int i = 0; i < pages.count(); ++i) {
        (pages.at(i).learnt > 0 ? learnt : rest).append(i);
    }
    const auto byLearnt = [&pages](int one, int other) {
        const Page &first = pages.at(one);
        const Page &second = pages.at(other);
        if (first.learnt != second.learnt) {
            return first.learnt > second.learnt;
        }
        if (first.frecency != second.frecency) {
            return first.frecency > second.frecency;
        }
        return one < other;
    };
    const auto byMatch = [&pages](int one, int other) {
        const Page &first = pages.at(one);
        const Page &second = pages.at(other);
        if (first.match != second.match) {
            return first.match < second.match;
        }
        if (first.frecency != second.frecency) {
            return first.frecency > second.frecency;
        }
        if (first.learnt != second.learnt) {
            return first.learnt > second.learnt;
        }
        return one < other;
    };
    const int lead = std::min({int(OmnibarModel::MaxLearnt), learnt.count(), std::max(room, 0)});
    std::partial_sort(learnt.begin(), learnt.begin() + lead, learnt.end(), byLearnt);
    for (int i = lead; i < learnt.count(); ++i) {
        rest.append(learnt.at(i));
    }
    const int more = std::min(std::max(room, 0) - lead, rest.count());
    std::partial_sort(rest.begin(), rest.begin() + more, rest.end(), byMatch);
    return learnt.mid(0, lead) + rest.mid(0, more);
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

struct OmnibarModel::Cache
{
    Sites tabs;
    Sites bookmarks;
    HistoryPages history;
};

OmnibarModel::OmnibarModel(TabModel *tabs, BookmarkModel *bookmarks, HistoryModel *history,
                           DownloadModel *downloads, SearchSettings *search,
                           PrivacySettings *privacy, QObject *parent)
    : QAbstractListModel(parent)
    , m_tabs(tabs)
    , m_bookmarks(bookmarks)
    , m_history(history)
    , m_downloads(downloads)
    , m_search(search)
    , m_privacy(privacy)
    , m_cache(std::make_unique<Cache>())
{
    m_refresh.setSingleShot(true);
    m_refresh.setInterval(0);
    connect(&m_refresh, &QTimer::timeout, this, &OmnibarModel::rebuild);

    // Tab order picks site icon: moves count.
    const auto tabsChanged = [this]() {
        m_cache->tabs = Sites();
        sourceChanged();
    };
    connect(m_tabs, &TabModel::countChanged, this, tabsChanged);
    connect(m_tabs, &TabModel::dataChanged, this, tabsChanged);
    connect(m_tabs, &TabModel::rowsMoved, this, tabsChanged);
    connect(m_tabs, &TabModel::groupsChanged, this, &OmnibarModel::sourceChanged);
    connect(m_tabs, &TabModel::activeTabChanged, this, &OmnibarModel::sourceChanged);
    connect(m_bookmarks, &BookmarkModel::revisionChanged, this, [this]() {
        m_cache->bookmarks = Sites();
        sourceChanged();
    });
    // Not count: oldest dropped as new arrives keeps count same.
    connect(m_downloads, &DownloadModel::rowsInserted, this, &OmnibarModel::sourceChanged);
    connect(m_downloads, &DownloadModel::rowsRemoved, this, &OmnibarModel::sourceChanged);
    connect(m_downloads, &DownloadModel::dataChanged, this, &OmnibarModel::sourceChanged);
    // Not row signals: visit moves or inserts rows; title or icon change past shown rows emits
    // none.
    connect(m_history, &HistoryModel::entriesChanged, this, [this]() {
        m_cache->history = HistoryPages();
        sourceChanged();
    });
    connect(m_search, &SearchSettings::omnibarTabsChanged, this, &OmnibarModel::sourceChanged);
    connect(m_search, &SearchSettings::omnibarBookmarksChanged, this, &OmnibarModel::sourceChanged);
    connect(m_search, &SearchSettings::omnibarHistoryChanged, this, &OmnibarModel::sourceChanged);
    connect(m_search, &SearchSettings::omnibarDownloadsChanged, this, &OmnibarModel::sourceChanged);
}

OmnibarModel::~OmnibarModel() = default;

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
    switch (static_cast<Role>(role)) {
    case Role::Kind:
        return kindName(row.kind);
    case Role::Title:
        return row.title;
    case Role::Url:
        return row.url;
    case Role::Host:
        return row.host;
    case Role::MarkedTitle:
        return row.markedTitle;
    case Role::MarkedHost:
        return row.markedHost;
    case Role::Favicon:
        return row.favicon;
    case Role::TabId:
        return row.kind == OmnibarKind::Tab ? row.id : 0;
    case Role::GroupId:
        return row.groupId;
    case Role::GroupName:
        return row.groupName;
    case Role::GroupTabCount:
        return row.groupTabCount;
    case Role::Bookmarked:
        return row.bookmarked;
    case Role::DownloadId:
        return row.kind == OmnibarKind::Download ? row.id : 0;
    case Role::DownloadStatus:
        return row.downloadStatus;
    case Role::Progress:
        return row.progress;
    case Role::Date:
        return row.date;
    default:
        return {};
    }
}

QHash<int, QByteArray> OmnibarModel::roleNames() const
{
    return {
        {roleId(Role::Kind), QByteArrayLiteral("kind")},
        {roleId(Role::Title), QByteArrayLiteral("title")},
        {roleId(Role::Url), QByteArrayLiteral("url")},
        {roleId(Role::Host), QByteArrayLiteral("host")},
        {roleId(Role::MarkedTitle), QByteArrayLiteral("markedTitle")},
        {roleId(Role::MarkedHost), QByteArrayLiteral("markedHost")},
        {roleId(Role::Favicon), QByteArrayLiteral("favicon")},
        {roleId(Role::TabId), QByteArrayLiteral("tabId")},
        {roleId(Role::GroupId), QByteArrayLiteral("groupId")},
        {roleId(Role::GroupName), QByteArrayLiteral("groupName")},
        {roleId(Role::GroupTabCount), QByteArrayLiteral("groupTabCount")},
        {roleId(Role::Bookmarked), QByteArrayLiteral("bookmarked")},
        {roleId(Role::DownloadId), QByteArrayLiteral("downloadId")},
        {roleId(Role::DownloadStatus), QByteArrayLiteral("downloadStatus")},
        {roleId(Role::Progress), QByteArrayLiteral("progress")},
        {roleId(Role::Date), QByteArrayLiteral("date")},
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
    // Immediate: pane reads rows right after setting query.
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
    if (m_privacy->rememberHistory()) {
        m_history->recordInput(typed, url);
    }
}

qint64 OmnibarModel::frecency(int visitCount, bool bookmarked, qint64 lastUsed, qint64 now)
{
    const double lambda = std::log(2.0) / HalfLifeDays;
    const qint64 today = now / Day;
    // Future timestamp (clock set back) = today.
    const qint64 age = today - std::min(lastUsed, now) / Day;
    const double score =
        (bookmarked ? 100 : 50) * std::exp(-lambda * double(age)) * std::max(visitCount, 1);
    return today + qint64(std::floor(std::log(score) / lambda));
}

// Inactive: source changes ignored.
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

QList<OmnibarRow> OmnibarModel::emptyRows() const
{
    QList<OmnibarRow> rows;
    if (!m_search->omnibarBookmarks()) {
        return rows;
    }
    for (const BookmarkModel::Bookmark &bookmark : m_bookmarks->bookmarks()) {
        OmnibarRow row;
        row.kind = OmnibarKind::Bookmark;
        row.id = bookmark.id;
        row.title = orUrl(bookmark.title, bookmark.url);
        row.url = bookmark.url;
        row.host = SearchSettings::displayAddress(bookmark.url);
        row.markedTitle = row.title.toHtmlEscaped();
        row.markedHost = row.host.toHtmlEscaped();
        row.favicon = bookmark.favicon;
        row.bookmarked = true;
        rows.append(row);
    }
    return rows;
}

// First source holding url decides row action. Visits counted even if history isn't a source.
QList<OmnibarRow> OmnibarModel::pageRows(const SearchWords &words, int room) const
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const Search search{words, m_history->inputRanks(m_query, now)};
    const Sites &tabs = tabSites(m_cache->tabs, *m_tabs);
    const Sites &bookmarks = bookmarkSites(m_cache->bookmarks, *m_bookmarks);
    const HistoryPages &history = historyPages(m_cache->history, *m_history);
    Pages pages;
    if (m_search->omnibarTabs()) {
        addTabs(pages, *m_tabs, search, tabs);
    }
    if (m_search->omnibarBookmarks()) {
        addBookmarks(pages, *m_bookmarks, search, bookmarks);
    }
    addHistory(pages, history, m_search->omnibarHistory(), search);

    for (Page &page : pages.list) {
        score(page, search, bookmarks.hosts.contains(page.row.url), now);
    }

    // Marking costs: shown rows only.
    QList<OmnibarRow> rows;
    for (const int i : rank(pages.list, room)) {
        OmnibarRow row = pages.list.at(i).row;
        row.markedTitle = words.marked(row.title);
        row.markedHost = words.marked(row.host);
        if (row.favicon.isEmpty()) {
            row.favicon = siteIcon(row.host, {&tabs.icons, &bookmarks.icons, &history.icons});
        }
        rows.append(row);
    }
    return rows;
}

QList<OmnibarRow> OmnibarModel::downloadRows(const SearchWords &words) const
{
    QList<OmnibarRow> rows;
    if (!m_search->omnibarDownloads()) {
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
        row.host = SearchSettings::displayAddress(download.url);
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
