// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QSet>
#include <QString>
#include <QTimer>

namespace Salama {

class BookmarkModel;
class DownloadModel;
class HistoryModel;
class SearchWords;
class Settings;
class TabModel;

// What the omnibar's rows are made of, kept beside the model rather than inside it, as
// Tab and TabGroup are kept beside the tab model: the functions that gather and rank
// the rows are then OmnibarModel.cpp's own, and not more members of a class Qt's model
// interface already makes long.
enum class OmnibarKind
{
    Tab,
    Bookmark,
    History,
    Download
};

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
    // A tab or a page of the history that is also bookmarked.
    bool bookmarked = false;
    int downloadStatus = 0;
    int progress = 0;
    QDateTime date;

    friend bool operator==(const OmnibarRow &one, const OmnibarRow &other)
    {
        return one.kind == other.kind && one.id == other.id && one.title == other.title &&
               one.url == other.url && one.host == other.host && one.favicon == other.favicon &&
               one.groupId == other.groupId && one.groupName == other.groupName &&
               one.groupTabCount == other.groupTabCount && one.bookmarked == other.bookmarked &&
               one.downloadStatus == other.downloadStatus && one.progress == other.progress &&
               one.date == other.date;
    }

    friend bool operator!=(const OmnibarRow &one, const OmnibarRow &other)
    {
        return !(one == other);
    }
};

// What the address bar finds as it is typed into, as one list ranked the way Firefox's
// address bar ranks what it finds (docs/DECISIONS/0027-omnibar.md): the open tabs of
// every group, the bookmarks, the history and the downloads that hold every word typed.
//
// A page is one row however many of those it is in: an open tab, to switch to, before
// a bookmark, before a page of the history; a tab or a history page that is also
// bookmarked says so. The tab in front is never listed as one: it is the page the bar
// is over. Downloads are files rather than pages, and rows of their own.
//
// The pages are ranked in three steps, Firefox's:
//
//  * First, up to MaxLearnt pages chosen before after typing what is typed now, the
//    likeliest first (HistoryModel::inputRanks) -- even when the words are not in
//    them: "gh" leads where it led last time. Firefox's input history.
//  * Then the rest by how well the words match -- an address whose host begins with the
//    first word, then a title or address with a word that does, then the rest -- and
//    within that, by frecency(): how often and how lately a page was visited, a
//    bookmark weighing twice a page. The host coming first stands in for Firefox's
//    autofill, which completes the host typed into the bar.
//  * The downloads last, newest first, no more than MaxDownloads of them.
//
// No more than MaxRows in all: the pane shows what is likeliest, not everything that
// matched, as Firefox shows ten.
//
// Nothing is listed while nothing is typed, unless bookmarksWhenEmpty is set: then the
// bookmarks are, all of them in their own order, as sailfish-browser's new-tab overlay
// lists its favourites.
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
    Q_PROPERTY(int count READ count NOTIFY resultsChanged)

public:
    enum Role
    {
        // "tab", "bookmark", "history" or "download": what a tap on the row does.
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
        // Whether a tab or a page of the history is bookmarked too; a bookmark is.
        BookmarkedRole,
        // The download's own lasting id (DownloadModel::rowOf), its DownloadModel::Status
        // and its progress; 0 on the other kinds.
        DownloadIdRole,
        DownloadStatusRole,
        ProgressRole,
        // A page of the history's last visit, a download's start; invalid otherwise.
        DateRole
    };

    // How many rows there are at most, how many of them a download may take, and how
    // many pages chosen before are put first. The pane sits over half a screen above
    // the keyboard: eight rows is about what it shows without scrolling.
    static const int MaxRows = 8;
    static const int MaxDownloads = 2;
    static const int MaxLearnt = 3;

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

    // That what was typed led to the page chosen from the list, or gone to as typed,
    // for the address bar to put it first next time (HistoryModel::recordInput) --
    // unless the history is not to be kept (Settings::rememberHistory).
    Q_INVOKABLE void learn(const QString &typed, const QString &url);

    // How a page is ranked among those that match as well: Firefox's frecency
    // (nsNavHistory::CalculateFrecency), from what is known of the page -- how many
    // visits, and the day of the last, or for a bookmark never visited the day it was
    // added. Each visit weighs 50 and a bookmark's 100, halving every 30 days; a page
    // never visited counts as visited once. Written as Firefox writes it: the day,
    // counted from the epoch, on which that score would have worn down to 1 -- so
    // pages used as much on the same day tie, and keep the order they were found in.
    static qint64 frecency(int visitCount, bool bookmarked, qint64 lastUsed, qint64 now);

signals:
    void queryChanged();
    void bookmarksWhenEmptyChanged();
    void resultsChanged();

private:
    bool active() const;
    // A source changed: the rows are built again once the event loop comes round,
    // once for however many changes arrived together -- a page loading says so for
    // its address, its title, its icon and its visit.
    void sourceChanged();
    void rebuild();
    QList<OmnibarRow> collect() const;
    QList<OmnibarRow> emptyRows() const;
    QList<OmnibarRow> pageRows(const SearchWords &words, int room) const;
    QList<OmnibarRow> downloadRows(const SearchWords &words) const;

    TabModel *m_tabs;
    BookmarkModel *m_bookmarks;
    HistoryModel *m_history;
    DownloadModel *m_downloads;
    Settings *m_settings;
    QString m_query;
    bool m_bookmarksWhenEmpty = false;
    QList<OmnibarRow> m_rows;
    QTimer m_refresh;
};

} // namespace Salama
