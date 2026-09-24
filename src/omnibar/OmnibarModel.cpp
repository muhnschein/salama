// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "OmnibarModel.h"

#include "bookmarks/BookmarkModel.h"
#include "downloads/DownloadModel.h"
#include "history/HistoryModel.h"
#include "search/SearchWords.h"
#include "settings/Settings.h"
#include "tabs/TabModel.h"

#include <algorithm>

namespace Salama {

namespace {

const qint64 Day = qint64(24) * 60 * 60 * 1000;

QString orUrl(const QString &title, const QString &url)
{
    return title.isEmpty() ? url : title;
}

// Where a kind's total is kept.
std::size_t slot(OmnibarKind kind)
{
    return static_cast<std::size_t>(kind);
}

int rank(const SearchWords &words, const QString &host, const QString &title)
{
    if (words.prefixes(host)) {
        return 0;
    }
    return words.prefixesAWordOf(title) ? 1 : 2;
}

// Ranks the candidates, stable over the order they came in, and appends the first cap
// of them to the rows and their addresses to the listed ones. Returns how many there
// were.
int take(QList<OmnibarCandidate> candidates, int cap, QList<OmnibarRow> &rows,
         QSet<QString> &listed)
{
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const OmnibarCandidate &one, const OmnibarCandidate &other) {
                         return one.rank < other.rank;
                     });
    for (int i = 0; i < candidates.count() && i < cap; ++i) {
        rows.append(candidates.at(i).row);
        listed.insert(candidates.at(i).row.url);
    }
    return candidates.count();
}

int shown(const QList<OmnibarRow> &rows, OmnibarKind kind)
{
    return static_cast<int>(std::count_if(
        rows.cbegin(), rows.cend(), [kind](const OmnibarRow &row) { return row.kind == kind; }));
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
        {FaviconRole, QByteArrayLiteral("favicon")},
        {TabIdRole, QByteArrayLiteral("tabId")},
        {GroupIdRole, QByteArrayLiteral("groupId")},
        {GroupNameRole, QByteArrayLiteral("groupName")},
        {GroupTabCountRole, QByteArrayLiteral("groupTabCount")},
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

int OmnibarModel::tabCount() const
{
    return shown(m_rows, OmnibarKind::Tab);
}

int OmnibarModel::tabTotal() const
{
    return m_totals.at(slot(OmnibarKind::Tab));
}

int OmnibarModel::bookmarkCount() const
{
    return shown(m_rows, OmnibarKind::Bookmark);
}

int OmnibarModel::bookmarkTotal() const
{
    return m_totals.at(slot(OmnibarKind::Bookmark));
}

int OmnibarModel::historyCount() const
{
    return shown(m_rows, OmnibarKind::History);
}

int OmnibarModel::historyTotal() const
{
    return m_totals.at(slot(OmnibarKind::History));
}

int OmnibarModel::downloadCount() const
{
    return shown(m_rows, OmnibarKind::Download);
}

int OmnibarModel::downloadTotal() const
{
    return m_totals.at(slot(OmnibarKind::Download));
}

qint64 OmnibarModel::frecency(int visitCount, const QDateTime &lastVisit, qint64 now)
{
    const qint64 age = now - lastVisit.toMSecsSinceEpoch();
    int weight = 10;
    if (age <= 4 * Day) {
        weight = 100;
    } else if (age <= 14 * Day) {
        weight = 70;
    } else if (age <= 31 * Day) {
        weight = 50;
    } else if (age <= 90 * Day) {
        weight = 30;
    }
    return qint64(visitCount) * weight;
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
    Totals totals{};
    const QList<OmnibarRow> rows = collect(totals);
    const bool sameRows = std::equal(rows.cbegin(), rows.cend(), m_rows.cbegin(), m_rows.cend(),
                                     [](const OmnibarRow &one, const OmnibarRow &other) {
                                         return one.kind == other.kind && one.id == other.id;
                                     });
    bool resultsDiffer = totals != m_totals;
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
    } else {
        beginResetModel();
        m_rows = rows;
        endResetModel();
        resultsDiffer = true;
    }
    m_totals = totals;
    if (resultsDiffer) {
        emit resultsChanged();
    }
}

// The sections in the order the pane shows them, each left without what an earlier
// one lists.
QList<OmnibarRow> OmnibarModel::collect(Totals &totals) const
{
    QList<OmnibarRow> rows;
    if (!active()) {
        return rows;
    }
    const SearchWords words(m_query);
    QSet<QString> listed;
    if (!words.isEmpty() && m_settings->omnibarTabs()) {
        totals[slot(OmnibarKind::Tab)] = take(tabCandidates(words), TabCap, rows, listed);
    }
    if (m_settings->omnibarBookmarks()) {
        totals[slot(OmnibarKind::Bookmark)] =
            take(bookmarkCandidates(words, listed), BookmarkCap, rows, listed);
    }
    if (!words.isEmpty() && m_settings->omnibarHistory()) {
        totals[slot(OmnibarKind::History)] =
            take(historyCandidates(words, listed), HistoryCap, rows, listed);
    }
    if (!words.isEmpty() && m_settings->omnibarDownloads()) {
        totals[slot(OmnibarKind::Download)] =
            take(downloadCandidates(words), DownloadCap, rows, listed);
    }
    return rows;
}

// Every group's tabs but the one in front, most recently in front first. Stable, so
// tabs never yet in front keep the order the model has them in.
QList<OmnibarCandidate> OmnibarModel::tabCandidates(const SearchWords &words) const
{
    QList<const Tab *> tabs;
    for (const Tab &tab : m_tabs->tabs()) {
        if (tab.id != m_tabs->activeTabId() && words.matches({tab.title, tab.url})) {
            tabs.append(&tab);
        }
    }
    std::stable_sort(tabs.begin(), tabs.end(), [](const Tab *one, const Tab *other) {
        return one->lastActive > other->lastActive;
    });
    QList<OmnibarCandidate> candidates;
    for (const Tab *tab : tabs) {
        OmnibarCandidate candidate;
        candidate.row.kind = OmnibarKind::Tab;
        candidate.row.id = tab->id;
        candidate.row.title = orUrl(tab->title, tab->url);
        candidate.row.url = tab->url;
        candidate.row.host = Settings::displayAddress(tab->url);
        candidate.row.favicon = tab->favicon;
        candidate.row.groupId = tab->groupId;
        const int group = m_tabs->groupIndexOf(tab->groupId);
        candidate.row.groupName = group >= 0 ? m_tabs->groups().at(group).name : QString();
        candidate.row.groupTabCount = m_tabs->tabCountInGroup(tab->groupId);
        candidate.rank = rank(words, candidate.row.host, tab->title);
        candidates.append(candidate);
    }
    return candidates;
}

// In the bookmarks' own order. With nothing typed every bookmark matches, and none
// ranks above another.
QList<OmnibarCandidate> OmnibarModel::bookmarkCandidates(const SearchWords &words,
                                                         const QSet<QString> &listed) const
{
    QList<OmnibarCandidate> candidates;
    for (const BookmarkModel::Bookmark &bookmark : m_bookmarks->bookmarks()) {
        if (listed.contains(bookmark.url) || !words.matches({bookmark.title, bookmark.url})) {
            continue;
        }
        OmnibarCandidate candidate;
        candidate.row.kind = OmnibarKind::Bookmark;
        candidate.row.id = bookmark.id;
        candidate.row.title = orUrl(bookmark.title, bookmark.url);
        candidate.row.url = bookmark.url;
        candidate.row.host = Settings::displayAddress(bookmark.url);
        candidate.row.favicon = bookmark.favicon;
        candidate.rank = rank(words, candidate.row.host, bookmark.title);
        candidates.append(candidate);
    }
    return candidates;
}

// The whole table rather than the model's page of it, which comes newest first, and
// then by frecency: of two pages as often and as lately visited, the later first.
QList<OmnibarCandidate> OmnibarModel::historyCandidates(const SearchWords &words,
                                                        const QSet<QString> &listed) const
{
    struct Scored
    {
        OmnibarCandidate candidate;
        qint64 score;
    };
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<Scored> scored;
    for (const HistoryModel::Entry &entry : m_history->allEntries()) {
        if (listed.contains(entry.url) || !words.matches({entry.title, entry.url})) {
            continue;
        }
        OmnibarCandidate candidate;
        candidate.row.kind = OmnibarKind::History;
        candidate.row.id = entry.id;
        candidate.row.title = orUrl(entry.title, entry.url);
        candidate.row.url = entry.url;
        candidate.row.host = Settings::displayAddress(entry.url);
        candidate.row.date = entry.date;
        candidate.rank = rank(words, candidate.row.host, entry.title);
        scored.append(Scored{candidate, frecency(entry.visitCount, entry.date, now)});
    }
    std::stable_sort(scored.begin(), scored.end(), [](const Scored &one, const Scored &other) {
        return one.score > other.score;
    });
    QList<OmnibarCandidate> candidates;
    for (const Scored &one : scored) {
        candidates.append(one.candidate);
    }
    return candidates;
}

// Newest first, as the downloads list has them. The host is where the file came from.
QList<OmnibarCandidate> OmnibarModel::downloadCandidates(const SearchWords &words) const
{
    QList<OmnibarCandidate> candidates;
    for (const DownloadModel::Download &download : m_downloads->downloads()) {
        if (!words.matches({download.name, download.url})) {
            continue;
        }
        OmnibarCandidate candidate;
        candidate.row.kind = OmnibarKind::Download;
        candidate.row.id = download.id;
        candidate.row.title = orUrl(download.name, download.url);
        candidate.row.url = download.url;
        candidate.row.host = Settings::displayAddress(download.url);
        candidate.row.downloadStatus = static_cast<int>(download.status);
        candidate.row.progress = download.progress;
        candidate.row.date = QDateTime::fromMSecsSinceEpoch(download.started);
        candidate.rank = rank(words, candidate.row.host, download.name);
        candidates.append(candidate);
    }
    return candidates;
}

} // namespace Salama
