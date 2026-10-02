// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/history/declarativetabmodel.{h,cpp}
// (Copyright (c) 2013 Jolla Ltd., (c) 2021 Open Mobile Platform LLC, MPL-2.0).
// Differences: no web container coupling, and the model reports navigations through
// signals instead of writing history.
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

// Every open tab, in one list, whatever group it is in: the browsing page keeps one
// view per row of this model, so a tab changing group must not be a row removed and
// inserted. The grid shows one group at a time through groupTabs(), and the strip
// above it lists the groups through groups() (docs/DECISIONS/0015-tab-groups.md).
//
// More methods than SonarQube allows a class (cpp:S1448), and kept whole on purpose:
// the tab in front, the groups, what each page plays and which pages stay loaded are
// all read from and written to the one list of rows, and each change to one of them
// has to be told as a change to those rows, in order, from here.
class TabModel : public QAbstractListModel // NOSONAR(cpp:S1448) one list of rows, told from here
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int activeTabIndex READ activeTabIndex NOTIFY activeTabChanged)
    Q_PROPERTY(int activeTabId READ activeTabId NOTIFY activeTabChanged)
    Q_PROPERTY(QString activeUrl READ activeUrl NOTIFY activeTabDataChanged)
    Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY activeTabDataChanged)
    Q_PROPERTY(QString activeFavicon READ activeFavicon NOTIFY activeTabDataChanged)
    // The group the grid shows and new tabs open in. It follows the active tab, and
    // choosing another group brings that group's most recent tab to the front.
    Q_PROPERTY(
        int currentGroupId READ currentGroupId WRITE setCurrentGroupId NOTIFY currentGroupChanged)
    Q_PROPERTY(int currentGroupIndex READ currentGroupIndex NOTIFY currentGroupChanged)
    // What the tab in front is playing, and whether it is muted: what the navigation
    // bar's media controls show (docs/DECISIONS/0026-media-controls.md).
    Q_PROPERTY(int activeMediaState READ activeMediaState NOTIFY activeMediaChanged)
    Q_PROPERTY(bool activeMuted READ activeMuted NOTIFY activeMediaChanged)
    // What the page in front says of what it plays, as the cover shows it
    // (docs/DECISIONS/0037-cover-is-where-you-were.md): empty while it says nothing.
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
        // Whether the page keeps its view: the tab in front and the ones read most
        // recently, up to the limit (docs/DECISIONS/0016-five-live-pages.md).
        Live,
        // What the page is playing, a MediaState, as the tab's controls show it -- a
        // page behind the one in front that says it plays shows as paused, below -- and
        // whether the tab is muted. Neither is persisted: the one is the page's own, and
        // goes with it; the other is kept for as long as the tab is open
        // (docs/DECISIONS/0026-media-controls.md).
        Media,
        Muted
    };

    // Something with sound is playing on the page; or this browser paused it, and it
    // can be played again from here; or neither. Unscoped on purpose, as
    // CoverSettings::QuickAction is: QML reads these as `TabModel.MediaPlaying`, which
    // Qt 5.6 cannot do for a scoped enum (cpp:S3642).
    enum MediaState // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        NoMedia,
        MediaPlaying,
        MediaPaused
    };
    Q_ENUM(MediaState)

    // What a page says of what it plays: the title, the artist and the address of a
    // picture, as its Media Session has them, or a video's poster for the picture
    // (PageMedia). Kept only while the page plays something, as its state is.
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

    // A null persistence keeps the model in memory only (used by tests). An empty
    // thumbnail directory turns page previews off.
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

    // Returns the new tab id, or 0 when the url is handed to another app (tel:, sms:, ...).
    // The tab opens in the current group. A tab with no url is on the start page, and
    // has no page until one is opened in it (docs/DECISIONS/0032-start-page.md).
    Q_INVOKABLE int newTab(const QString &url);
    // The same, in the default group, which becomes the current one: where a link
    // shared from another application opens, whatever group was being read
    // (docs/DECISIONS/0042-share-target.md).
    Q_INVOKABLE int newTabInDefaultGroup(const QString &url);
    // A tab for a link, opened in the current group behind the tab in front, which
    // stays there: the link sheet's Background tab (docs/DECISIONS/0046-link-menu.md).
    // It is named by the link's text until its page says otherwise, and has no view until
    // it first comes to the front, as a restored tab has none
    // (docs/DECISIONS/0003-one-webview-per-tab.md). Returns the new tab id, or 0 for no
    // address, or one another application takes.
    Q_INVOKABLE int newTabBehind(const QString &url, const QString &title);
    Q_INVOKABLE void activateTab(int index);
    Q_INVOKABLE bool activateTabById(int tabId);
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE void closeTabById(int tabId);
    // Reorder, from the grid. The active tab stays active wherever it lands.
    Q_INVOKABLE void moveTab(int from, int to);
    Q_INVOKABLE void closeActiveTab();
    Q_INVOKABLE void closeAllTabs();
    Q_INVOKABLE int indexOf(int tabId) const;
    // The open tab showing this address, in whichever group, 0 when there is none; of
    // several, the one in front most recently. The cover's bookmark action brings that
    // tab to the front rather than open the page again
    // (docs/DECISIONS/0029-quick-action.md).
    Q_INVOKABLE int tabIdForUrl(const QString &url) const;
    // The name of the group a tab is in, empty for an unnamed group or no such tab.
    Q_INVOKABLE QString groupNameOf(int tabId) const;

    // Called by the view as the engine reports page state.
    Q_INVOKABLE void updateUrl(int tabId, const QString &url);
    Q_INVOKABLE void updateTitle(int tabId, const QString &title);
    Q_INVOKABLE void updateFavicon(int tabId, const QString &favicon);
    // Back to the start page, as back from the first page opened from it goes: the tab
    // gives up its url, title, icon and preview, and its page with them.
    Q_INVOKABLE void showStartPage(int tabId);

    // Where the view should write this tab's next page preview. Each call returns a
    // fresh name so the grabbed image is never hidden behind a cached one, and the
    // previous file is removed once the new path is handed back through
    // updateThumbnail(). Empty when previews are off.
    Q_INVOKABLE QString thumbnailPath(int tabId);
    Q_INVOKABLE void updateThumbnail(int tabId, const QString &path);
    // A grabbed picture, the image of a QQuickItemGrabResult, written to a fresh
    // thumbnailPath() away from the GUI thread and handed to updateThumbnail() once it
    // is on disk. A write overtaken by a later one for the same tab, or finished after
    // its tab has closed, is thrown away. False when nothing will be written.
    Q_INVOKABLE bool storeThumbnail(int tabId, const QVariant &image);
    ThumbnailWriter *thumbnailWriter() const
    {
        return m_thumbnailWriter;
    }

    // What a page is playing, as PageMedia reads it from the page, and whether its tab
    // is muted. A tab whose page is not kept loaded plays nothing.
    MediaState mediaState(int tabId) const;
    // The same, as the tab's controls show it. A page behind the one in front can say
    // it plays, but its document is hidden and the engine holds a hidden document's
    // media until it is shown again: to anyone looking it is paused, and it plays
    // when it is brought to the front.
    MediaState shownMediaState(int tabId) const;
    void setMediaState(int tabId, MediaState state);
    // What the page says of what it plays. Setting it is refused for a page that plays
    // nothing, and it goes as the page stops playing: what it says goes with what it
    // plays.
    MediaMetadata mediaMetadata(int tabId) const;
    void setMediaMetadata(int tabId, const MediaMetadata &metadata);
    bool isMuted(int tabId) const;
    void setMuted(int tabId, bool muted);

    // Tab groups. There is always one default group, first in the list, which can be
    // neither renamed nor removed.
    const QList<TabGroup> &groups() const;
    int groupIndexOf(int groupId) const;
    int defaultGroupId() const;
    int tabCountInGroup(int groupId) const;
    int currentGroupId() const;
    int currentGroupIndex() const;
    void setCurrentGroupId(int groupId);
    // Returns the new group's id. The new group goes last and becomes the current one.
    int addGroup(const QString &name);
    // Refused for the default group.
    void renameGroup(int groupId, const QString &name);
    // Closes the group's tabs and removes it. Refused for the default group.
    bool removeGroup(int groupId);
    // Puts the group's tabs in the default group, open as they were, and removes it.
    // Refused for the default group.
    bool ungroup(int groupId);
    // Puts the group at one place in the order at another. The default group is first
    // and stays there: it is not moved, and nothing is moved in front of it.
    bool moveGroup(int from, int to);
    // Puts a tab in another group. Its row in this model does not move, so the view
    // behind it stays; its place in the group is after the tabs already there.
    bool moveTabToGroup(int tabId, int groupId);
    // The previews of the group's tabs, most recently in front first, at most this
    // many; a tab with no picture is an empty string rather than a gap.
    QStringList groupThumbnails(int groupId, int limit) const;

    // How many tabs keep their page loaded, 0 for all of them. The browser keeps
    // LiveTabLimit, as Jolla's does (docs/DECISIONS/0016-five-live-pages.md).
    static const int LiveTabLimit = 5;
    int liveTabLimit() const;
    void setLiveTabLimit(int limit);

    // The views of this model the grid, the strip and the closed-tabs panel are
    // built on.
    GroupTabModel *groupTabs() const;
    TabGroupModel *groupModel() const;
    ClosedTabModel *closedTabs() const;

    static bool isExternalUrl(const QString &url);

signals:
    void countChanged();
    // The tab in front is about to be another: told before anything else hears of it,
    // while the page being left is still the one on the screen (PageMedia).
    void activeTabLeaving(int tabId);
    void activeTabChanged();
    // The groups' pictures have changed: a tab opened or closed, one came to the
    // front, or a preview was captured.
    void recentTabsChanged();
    void activeTabDataChanged();
    // What the tab in front plays, or whether it is muted, has changed -- or another tab
    // has come to the front.
    void activeMediaChanged();
    void tabAdded(int tabId);
    void tabClosed(int tabId);
    // Wired to the history model.
    void visited(const QString &url);
    void titleUpdated(const QString &url, const QString &title);
    void faviconUpdated(const QString &url, const QString &favicon);
    void currentGroupChanged();
    // A group was added, renamed, moved or removed, or a tab changed group.
    void groupsChanged();

private:
    void load();
    // A tab at the end of the current group, not brought to the front; its id, or 0.
    int insertTab(const QString &url, const QString &title);
    void ensureGroups();
    // Which tabs keep their views, and the same recomputed with the rows that changed
    // told; the constructor takes the set alone, there being no rows to tell yet.
    QSet<int> liveSet() const;
    void refreshLive();
    void setActiveTab(int tabId);
    // The tab and the group, each without following the other.
    void applyActiveTab(int tabId);
    void applyCurrentGroup(int groupId);
    // The tab to bring to the front when the active one goes: the nearest in its own
    // group, then the most recent anywhere.
    int successorOf(int index) const;
    int mostRecentTabId(int groupId) const;
    int groupRowFor(int index) const;
    // Marks the tab in front as the most recent one, and tells the groups' pictures.
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
    // Only the tabs whose page plays something, and only the muted tabs.
    QHash<int, MediaState> m_media;
    QSet<int> m_muted;
    QHash<int, MediaMetadata> m_metadata;
    int m_liveLimit = 0;
    int m_activeTabId = 0;
    int m_currentGroupId = 0;
    int m_nextTabId = 1;
    int m_nextGroupId = 1;
    // Counts activations rather than milliseconds: the order is all anyone reads, and a
    // counter cannot be turned around by a clock that steps backwards.
    qint64 m_activationClock = 0;
    int m_thumbnailCounter = 0;
    ThumbnailWriter *m_thumbnailWriter;
    // The newest write asked for, per tab, until it has finished.
    QHash<int, QString> m_pendingThumbnails;
};

} // namespace Salama
