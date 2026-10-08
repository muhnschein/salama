// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/history/declarativetabmodel.{h,cpp}
// (Copyright (c) 2013 Jolla Ltd., (c) 2021 Open Mobile Platform LLC, MPL-2.0).
#pragma once

#include "ModelRoles.h"

#include "Tab.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Salama {

class ClosedTabModel;
class GroupTabModel;
class TabGroupModel;
class TabPersistence;
class ThumbnailWriter;

// All tabs, every group, one list: view per row, so group change must not remove/insert rows.
// Over S1448 method count on purpose: all state changes signal as row changes, from here.
class TabModel : public QAbstractListModel // NOSONAR(cpp:S1448) one list of rows, told from here
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int activeTabIndex READ activeTabIndex NOTIFY activeTabChanged)
    Q_PROPERTY(int activeTabId READ activeTabId NOTIFY activeTabChanged)
    Q_PROPERTY(QString activeUrl READ activeUrl NOTIFY activeTabDataChanged)
    Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY activeTabDataChanged)
    Q_PROPERTY(QString activeFavicon READ activeFavicon NOTIFY activeTabDataChanged)
    Q_PROPERTY(
        int currentGroupId READ currentGroupId WRITE setCurrentGroupId NOTIFY currentGroupChanged)
    Q_PROPERTY(int currentGroupIndex READ currentGroupIndex NOTIFY currentGroupChanged)
    Q_PROPERTY(int activeMediaState READ activeMediaState NOTIFY activeMediaChanged)
    Q_PROPERTY(bool activeMuted READ activeMuted NOTIFY activeMediaChanged)
    Q_PROPERTY(QString activeMediaTitle READ activeMediaTitle NOTIFY activeMediaChanged)
    Q_PROPERTY(QString activeMediaArtist READ activeMediaArtist NOTIFY activeMediaChanged)
    Q_PROPERTY(QString activeMediaArtwork READ activeMediaArtwork NOTIFY activeMediaChanged)

public:
    enum class Role
    {
        TabId = Qt::UserRole + 1,
        Url,
        Title,
        Favicon,
        Thumbnail,
        Active,
        Group,
        // Keeps view: front + most recent, up to limit.
        Live,
        // As shown (behind-front plays as paused). Not persisted.
        Media,
        Muted
    };

    // Unscoped: Qt 5.6 QML reads no scoped enum.
    enum MediaState // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        NoMedia,
        MediaPlaying,
        MediaPaused
    };
    Q_ENUM(MediaState)

    // Kept only while playing.
    struct MediaMetadata
    {
        QString title;
        QString artist;
        QString artwork;

        friend bool operator==(const MediaMetadata &lhs, const MediaMetadata &rhs)
        {
            return lhs.title == rhs.title && lhs.artist == rhs.artist && lhs.artwork == rhs.artwork;
        }

        friend bool operator!=(const MediaMetadata &lhs, const MediaMetadata &rhs)
        {
            return !(lhs == rhs);
        }
    };

    // Null persistence = memory only. Empty thumbnail dir = no previews.
    explicit TabModel(TabPersistence *persistence, QString thumbnailDirectory = QString(),
                      QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    int activeTabIndex() const;
    int activeTabId() const;
    QString activeUrl() const;
    QString activeTitle() const;
    QString activeFavicon() const;
    int activeMediaState() const;
    bool activeMuted() const;
    QString activeMediaTitle() const;
    QString activeMediaArtist() const;
    QString activeMediaArtwork() const;
    const QList<Tab> &tabs() const;

    // 0 if url external (tel:, sms:, ...). No url = start page.
    Q_INVOKABLE int newTab(const QString &url);
    Q_INVOKABLE int newTabInDefaultGroup(const QString &url);
    // No view until fronted.
    Q_INVOKABLE int newTabBehind(const QString &url, const QString &title);
    Q_INVOKABLE void activateTab(int index);
    Q_INVOKABLE bool activateTabById(int tabId);
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE void closeTabById(int tabId);
    Q_INVOKABLE void moveTab(int from, int to);
    Q_INVOKABLE void closeActiveTab();
    Q_INVOKABLE void closeAllTabs();
    Q_INVOKABLE int indexOf(int tabId) const;
    Q_INVOKABLE int tabIdForUrl(const QString &url) const;
    Q_INVOKABLE QString groupNameOf(int tabId) const;

    Q_INVOKABLE void updateUrl(int tabId, const QString &url);
    Q_INVOKABLE void updateTitle(int tabId, const QString &title);
    Q_INVOKABLE void updateFavicon(int tabId, const QString &favicon);
    Q_INVOKABLE void showStartPage(int tabId);

    // Fresh name each call so cache never masks new grab.
    Q_INVOKABLE QString thumbnailPath(int tabId);
    Q_INVOKABLE void updateThumbnail(int tabId, const QString &path);
    // Encodes off GUI thread. Superseded or post-close writes dropped.
    Q_INVOKABLE bool storeThumbnail(int tabId, const QVariant &image);
    ThumbnailWriter *thumbnailWriter() const
    {
        return m_thumbnailWriter;
    }

    MediaState mediaState(int tabId) const;
    // Engine holds hidden document's media: playing page behind front shows paused.
    MediaState shownMediaState(int tabId) const;
    void setMediaState(int tabId, MediaState state);
    MediaMetadata mediaMetadata(int tabId) const;
    void setMediaMetadata(int tabId, const MediaMetadata &metadata);
    bool isMuted(int tabId) const;
    void setMuted(int tabId, bool muted);

    // Default group: first, can't rename/remove/move.
    const QList<TabGroup> &groups() const;
    int groupIndexOf(int groupId) const;
    int defaultGroupId() const;
    int tabCountInGroup(int groupId) const;
    int currentGroupId() const;
    int currentGroupIndex() const;
    void setCurrentGroupId(int groupId);
    int addGroup(const QString &name);
    void renameGroup(int groupId, const QString &name);
    bool removeGroup(int groupId);
    bool ungroup(int groupId);
    bool moveGroup(int from, int to);
    // Row stays, so view stays.
    bool moveTabToGroup(int tabId, int groupId);
    QStringList groupThumbnails(int groupId, int limit) const;

    // 0 = all.
    static const int LiveTabLimit = 5;
    int liveTabLimit() const;
    void setLiveTabLimit(int limit);

    GroupTabModel *groupTabs() const;
    TabGroupModel *groupModel() const;
    ClosedTabModel *closedTabs() const;

    static bool isExternalUrl(const QString &url);

signals:
    void countChanged();
    // Emitted first, while leaving page still on screen.
    void activeTabLeaving(int tabId);
    void activeTabChanged();
    void recentTabsChanged();
    void activeTabDataChanged();
    void activeMediaChanged();
    void tabAdded(int tabId);
    void tabClosed(int tabId);
    void visited(const QString &url);
    void titleUpdated(const QString &url, const QString &title);
    void faviconUpdated(const QString &url, const QString &favicon);
    void currentGroupChanged();
    void groupsChanged();

private:
    void load();
    int insertTab(const QString &url, const QString &title);
    void ensureGroups();
    QSet<int> liveSet() const;
    void refreshLive();
    void setActiveTab(int tabId);
    void applyActiveTab(int tabId);
    void applyCurrentGroup(int groupId);
    int successorOf(int index) const;
    int mostRecentTabId(int groupId) const;
    int groupRowFor(int index) const;
    void stampActive();
    void notifyRow(int index, Role role);
    void persist(const Tab &tab) const;
    void discardThumbnail(const QString &path) const;
    void thumbnailWritten(int tabId, const QString &path, bool saved);

    TabPersistence *m_persistence;
    QString m_thumbnailDirectory;
    QList<Tab> m_tabs;
    QList<TabGroup> m_groups;
    QList<int> m_awaitingFirstUrl;
    GroupTabModel *m_groupTabs;
    TabGroupModel *m_groupModel;
    ClosedTabModel *m_closedTabs;
    QSet<int> m_liveIds;
    QHash<int, MediaState> m_media;
    QSet<int> m_muted;
    QHash<int, MediaMetadata> m_metadata;
    int m_liveLimit = 0;
    int m_activeTabId = 0;
    int m_currentGroupId = 0;
    int m_nextTabId = 1;
    int m_nextGroupId = 1;
    // Counter not ms: immune to clock stepping back.
    qint64 m_activationClock = 0;
    int m_thumbnailCounter = 0;
    ThumbnailWriter *m_thumbnailWriter;
    QHash<int, QString> m_pendingThumbnails;
};

} // namespace Salama
