// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

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
class PrivacySettings;
class SearchSettings;
class TabModel;

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
    QString markedTitle;
    QString markedHost;
    QString favicon;
    int groupId = 0;
    QString groupName;
    int groupTabCount = 0;
    bool bookmarked = false;
    int downloadStatus = 0;
    int progress = 0;
    QDateTime date;

    friend bool operator==(const OmnibarRow &one, const OmnibarRow &other)
    {
        return one.kind == other.kind && one.id == other.id && one.title == other.title &&
               one.url == other.url && one.host == other.host &&
               one.markedTitle == other.markedTitle && one.markedHost == other.markedHost &&
               one.favicon == other.favicon && one.groupId == other.groupId &&
               one.groupName == other.groupName && one.groupTabCount == other.groupTabCount &&
               one.bookmarked == other.bookmarked && one.downloadStatus == other.downloadStatus &&
               one.progress == other.progress && one.date == other.date;
    }

    friend bool operator!=(const OmnibarRow &one, const OmnibarRow &other)
    {
        return !(one == other);
    }
};

// Address bar results, Firefox-ranked: learnt pages first (even unmatched), then match
// quality (host prefix first, stands in for autofill), then frecency(); downloads last. Same
// kind+id rebuild updates in place: no reset under finger.
class OmnibarModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool bookmarksWhenEmpty READ bookmarksWhenEmpty WRITE setBookmarksWhenEmpty NOTIFY
                   bookmarksWhenEmptyChanged)
    Q_PROPERTY(int count READ count NOTIFY resultsChanged)

public:
    enum class Role
    {
        Kind = Qt::UserRole + 1,
        Title,
        Url,
        Host,
        // Rest escaped: titles may hold no markup.
        MarkedTitle,
        MarkedHost,
        Favicon,
        TabId,
        GroupId,
        GroupName,
        GroupTabCount,
        Bookmarked,
        DownloadId,
        DownloadStatus,
        Progress,
        Date
    };

    // ~ what pane shows above keyboard without scrolling.
    static const int MaxRows = 8;
    static const int MaxDownloads = 2;
    static const int MaxLearnt = 3;

    OmnibarModel(TabModel *tabs, BookmarkModel *bookmarks, HistoryModel *history,
                 DownloadModel *downloads, SearchSettings *search, PrivacySettings *privacy,
                 QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString query() const;
    void setQuery(const QString &query);
    bool bookmarksWhenEmpty() const;
    void setBookmarksWhenEmpty(bool on);

    int count() const;

    Q_INVOKABLE void learn(const QString &typed, const QString &url) const;

    // Firefox CalculateFrecency: visit 50, bookmark 100, halves every 30 days. Returns epoch day
    // score decays to 1, so same-day equal use ties.
    static qint64 frecency(int visitCount, bool bookmarked, qint64 lastUsed, qint64 now);

signals:
    void queryChanged();
    void bookmarksWhenEmptyChanged();
    void resultsChanged();

private:
    bool active() const;
    // Coalesced: one page load signals several changes.
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
    SearchSettings *m_search;
    PrivacySettings *m_privacy;
    QString m_query;
    bool m_bookmarksWhenEmpty = false;
    QList<OmnibarRow> m_rows;
    QTimer m_refresh;
};

} // namespace Salama
