// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QSet>
#include <QString>
#include <QTimer>
#include <array>

namespace Salama {

class BookmarkModel;
class DownloadModel;
class HistoryModel;
class SearchWords;
class Settings;
class TabModel;

// What the omnibar's rows are made of, kept beside the model rather than inside it, as
// Tab and TabGroup are kept beside the tab model: the functions that rank, cap and count
// the rows are then OmnibarModel.cpp's own, and not more members of a class Qt's model
// interface already makes long.
enum class OmnibarKind
{
    Tab,
    Bookmark,
    History,
    Download
};

// How many kinds there are: the length of the per-kind totals.
const std::size_t OmnibarKinds = 4;

struct OmnibarRow
{
    OmnibarKind kind = OmnibarKind::Tab;
    int id = 0;
    QString title;
    QString url;
    QString host;
    QString favicon;
    int groupId = 0;
    QString groupName;
    int groupTabCount = 0;
    int downloadStatus = 0;
    int progress = 0;
    QDateTime date;

    friend bool operator==(const OmnibarRow &one, const OmnibarRow &other)
    {
        return one.kind == other.kind && one.id == other.id && one.title == other.title &&
               one.url == other.url && one.host == other.host && one.favicon == other.favicon &&
               one.groupId == other.groupId && one.groupName == other.groupName &&
               one.groupTabCount == other.groupTabCount &&
               one.downloadStatus == other.downloadStatus && one.progress == other.progress &&
               one.date == other.date;
    }

    friend bool operator!=(const OmnibarRow &one, const OmnibarRow &other)
    {
        return !(one == other);
    }
};

// A row with the rank it is sorted by, before it is capped.
struct OmnibarCandidate
{
    OmnibarRow row;
    int rank = 0;
};

// What the address bar finds as it is typed into: the open tabs, the bookmarks, the
// history and the downloads that hold every word typed, one section of each, in that
// order -- the pane above the bar lists them under a heading per kind, as piirit lists
// what its search finds (docs/DECISIONS/0027-omnibar.md).
//
// Within a section the rows are ranked, and the rank is stable over the source's own
// order: an address whose host begins with the first word first, then a title with a
// word that does, then the rest. The source's order is the tabs most recently in front
// first, the bookmarks as their list has them, the history by how often and how lately
// a page was visited (frecency()), and the downloads newest first. The tab in front is
// never listed: it is the page the bar is over. A bookmark open in a tab listed above
// it is left out, and so is a page of the history that is listed as either: one row a
// page, the nearest to hand. What is left is what the totals count.
//
// Nothing is listed while nothing is typed, unless bookmarksWhenEmpty is set: then the
// bookmarks are, as sailfish-browser's new-tab overlay lists its favourites.
//
// The rows are a snapshot of their sources, built again when the query changes and --
// while there is a query or the empty bookmarks are asked for -- when a source does.
// A rebuild that comes to the same rows, kind for kind and id for id, changes them in
// place, so a download's progress or a tab's favicon arriving never tears down the row
// under the reader's finger; only different rows reset the list.
class OmnibarModel : public QAbstractListModel
{
    Q_OBJECT
    // Trimmed as it is set.
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool bookmarksWhenEmpty READ bookmarksWhenEmpty WRITE setBookmarksWhenEmpty NOTIFY
                   bookmarksWhenEmptyChanged)
    // How many rows there are, and per section how many are shown of how many matched:
    // a section's heading reads "History (10 of 34)".
    Q_PROPERTY(int count READ count NOTIFY resultsChanged)
    Q_PROPERTY(int tabCount READ tabCount NOTIFY resultsChanged)
    Q_PROPERTY(int tabTotal READ tabTotal NOTIFY resultsChanged)
    Q_PROPERTY(int bookmarkCount READ bookmarkCount NOTIFY resultsChanged)
    Q_PROPERTY(int bookmarkTotal READ bookmarkTotal NOTIFY resultsChanged)
    Q_PROPERTY(int historyCount READ historyCount NOTIFY resultsChanged)
    Q_PROPERTY(int historyTotal READ historyTotal NOTIFY resultsChanged)
    Q_PROPERTY(int downloadCount READ downloadCount NOTIFY resultsChanged)
    Q_PROPERTY(int downloadTotal READ downloadTotal NOTIFY resultsChanged)

public:
    enum Role
    {
        // "tab", "bookmark", "history" or "download": what the list is sectioned by.
        KindRole = Qt::UserRole + 1,
        // The page's or the file's name, the address when it has none.
        TitleRole,
        UrlRole,
        // Settings::displayAddress() of the url: for a download, the host it came from.
        HostRole,
        FaviconRole,
        // The tab's id, and its group as the grid's search gives it; 0 and empty on
        // the other kinds.
        TabIdRole,
        GroupIdRole,
        GroupNameRole,
        GroupTabCountRole,
        // The download's own lasting id (DownloadModel::rowOf), its DownloadModel::Status
        // and its progress; 0 on the other kinds.
        DownloadIdRole,
        DownloadStatusRole,
        ProgressRole,
        // A page of the history's last visit, a download's start; invalid otherwise.
        DateRole
    };

    // How many rows a section shows at most. The pane sits over half a screen above
    // the keyboard: ten rows is more than it shows without scrolling, and a first row
    // the reader has to scroll past three sections of fifty to find is no suggestion.
    // Downloads fewer, being the kind looked for least from an address bar. The
    // heading says how many more there were.
    static const int TabCap = 10;
    static const int BookmarkCap = 10;
    static const int HistoryCap = 10;
    static const int DownloadCap = 5;

    OmnibarModel(TabModel *tabs, BookmarkModel *bookmarks, HistoryModel *history,
                 DownloadModel *downloads, Settings *settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString query() const;
    void setQuery(const QString &query);
    bool bookmarksWhenEmpty() const;
    void setBookmarksWhenEmpty(bool on);

    int count() const;
    int tabCount() const;
    int tabTotal() const;
    int bookmarkCount() const;
    int bookmarkTotal() const;
    int historyCount() const;
    int historyTotal() const;
    int downloadCount() const;
    int downloadTotal() const;

    // How a page of the history is ordered before it is ranked: the visits, weighted
    // by how long ago the last one was -- 100 within four days, 70 within a fortnight,
    // 50 within a month, 30 within three months, 10 before -- as Firefox's frecency
    // weighs visits by their age, without keeping every visit to weigh.
    static qint64 frecency(int visitCount, const QDateTime &lastVisit, qint64 now);

signals:
    void queryChanged();
    void bookmarksWhenEmptyChanged();
    void resultsChanged();

private:
    using Totals = std::array<int, OmnibarKinds>;

    bool active() const;
    // A source changed: the rows are built again once the event loop comes round,
    // once for however many changes arrived together -- a page loading says so for
    // its address, its title, its icon and its visit.
    void sourceChanged();
    void rebuild();
    QList<OmnibarRow> collect(Totals &totals) const;
    QList<OmnibarCandidate> tabCandidates(const SearchWords &words) const;
    QList<OmnibarCandidate> bookmarkCandidates(const SearchWords &words,
                                               const QSet<QString> &listed) const;
    QList<OmnibarCandidate> historyCandidates(const SearchWords &words,
                                              const QSet<QString> &listed) const;
    QList<OmnibarCandidate> downloadCandidates(const SearchWords &words) const;

    TabModel *m_tabs;
    BookmarkModel *m_bookmarks;
    HistoryModel *m_history;
    DownloadModel *m_downloads;
    Settings *m_settings;
    QString m_query;
    bool m_bookmarksWhenEmpty = false;
    QList<OmnibarRow> m_rows;
    Totals m_totals{};
    QTimer m_refresh;
};

} // namespace Salama
