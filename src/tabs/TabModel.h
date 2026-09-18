// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/history/declarativetabmodel.{h,cpp}
// (Copyright (c) 2013 Jolla Ltd., (c) 2021 Open Mobile Platform LLC, MPL-2.0).
// Differences: no web container coupling, private tabs are a per-tab flag, and the
// model reports navigations through signals instead of writing history.
#pragma once

#include "Tab.h"

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

namespace Salama {

class ClosedTabModel;
class GroupTabModel;
class TabGroupModel;
class TabPersistence;

// Every open tab, in one list, whatever group it is in: the browsing page keeps one
// view per row of this model, so a tab changing group must not be a row removed and
// inserted. The grid shows one group at a time through groupTabs(), and the strip
// above it lists the groups through groups() (docs/DECISIONS/0015-tab-groups.md).
class TabModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int activeTabIndex READ activeTabIndex NOTIFY activeTabChanged)
    Q_PROPERTY(int activeTabId READ activeTabId NOTIFY activeTabChanged)
    Q_PROPERTY(bool activeIsPrivate READ activeIsPrivate NOTIFY activeTabChanged)
    Q_PROPERTY(QString activeUrl READ activeUrl NOTIFY activeTabDataChanged)
    Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY activeTabDataChanged)
    Q_PROPERTY(QString activeFavicon READ activeFavicon NOTIFY activeTabDataChanged)
    // The open tabs' previews, most recently in front first. What the cover draws its
    // field from (docs/DECISIONS/0014-cover-is-the-tab-count.md); a tab with no picture
    // is an empty string rather than a gap, so the list is always as long as count.
    Q_PROPERTY(QStringList recentThumbnails READ recentThumbnails NOTIFY recentTabsChanged)
    // The group the grid shows and new tabs open in. It follows the active tab, and
    // choosing another group brings that group's most recent tab to the front.
    Q_PROPERTY(
        int currentGroupId READ currentGroupId WRITE setCurrentGroupId NOTIFY currentGroupChanged)
    Q_PROPERTY(int currentGroupIndex READ currentGroupIndex NOTIFY currentGroupChanged)

public:
    enum Role
    {
        TabIdRole = Qt::UserRole + 1,
        UrlRole,
        TitleRole,
        FaviconRole,
        ThumbnailRole,
        PrivateRole,
        ActiveRole,
        GroupRole,
        // Whether the page keeps its view: the tab in front and the ones read most
        // recently, up to the limit (docs/DECISIONS/0016-five-live-pages.md).
        LiveRole
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
    bool activeIsPrivate() const;
    QString activeUrl() const;
    QString activeTitle() const;
    QString activeFavicon() const;
    QStringList recentThumbnails() const;
    const QList<Tab> &tabs() const;

    // Returns the new tab id, or 0 when the url is handed to another app (tel:, sms:, ...).
    // The tab opens in the current group; a private one in the private group, which
    // becomes current with it.
    Q_INVOKABLE int newTab(const QString &url, bool isPrivate = false);
    Q_INVOKABLE void activateTab(int index);
    Q_INVOKABLE bool activateTabById(int tabId);
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE void closeTabById(int tabId);
    // Reorder, from the grid. The active tab stays active wherever it lands.
    Q_INVOKABLE void moveTab(int from, int to);
    Q_INVOKABLE void closeActiveTab();
    Q_INVOKABLE void closeAllTabs();
    Q_INVOKABLE int indexOf(int tabId) const;

    // Called by the view as the engine reports page state.
    Q_INVOKABLE void updateUrl(int tabId, const QString &url);
    Q_INVOKABLE void updateTitle(int tabId, const QString &title);
    Q_INVOKABLE void updateFavicon(int tabId, const QString &favicon);

    // Where the view should write this tab's next page preview. Each call returns a
    // fresh name so the grabbed image is never hidden behind a cached one, and the
    // previous file is removed once the new path is handed back through
    // updateThumbnail(). Empty for a private tab, whose preview is never written to
    // disk, and when previews are off.
    Q_INVOKABLE QString thumbnailPath(int tabId);
    Q_INVOKABLE void updateThumbnail(int tabId, const QString &path);

    // Tab groups. There is always one private group, first in the list, and one
    // default group right after it; neither can be renamed or removed.
    const QList<TabGroup> &groups() const;
    int groupIndexOf(int groupId) const;
    int privateGroupId() const;
    int defaultGroupId() const;
    int tabCountInGroup(int groupId) const;
    int currentGroupId() const;
    int currentGroupIndex() const;
    void setCurrentGroupId(int groupId);
    // Returns the new group's id. The new group goes last and becomes the current one.
    int addGroup(const QString &name);
    // Refused for the private group and the default one.
    void renameGroup(int groupId, const QString &name);
    // Closes the group's tabs and removes it. Refused for the private group and the
    // default one.
    bool removeGroup(int groupId);
    // Puts a tab in another group. Its row in this model does not move, so the view
    // behind it stays; its place in the group is after the tabs already there. A tab
    // never crosses into or out of the private group: private is a property of its
    // view, not something a view can be given later.
    bool moveTabToGroup(int tabId, int groupId);

    // How many tabs keep their page loaded, 0 for all of them.
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
    void activeTabChanged();
    // The cover's list has changed: a tab opened or closed, one came to the front, or
    // a preview was captured.
    void recentTabsChanged();
    void activeTabDataChanged();
    void tabAdded(int tabId);
    void tabClosed(int tabId);
    // Emitted for non-private tabs only; wired to the history model.
    void visited(const QString &url);
    void titleUpdated(const QString &url, const QString &title);
    void faviconUpdated(const QString &url, const QString &favicon);
    void currentGroupChanged();
    // A group was added, renamed or removed, or a tab changed group.
    void groupsChanged();

private:
    void load();
    void ensureGroups();
    void ensureGroupKinds();
    void fileTabsByKind();
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
    // The private group and the default one: neither renamed nor removed.
    bool isFixedGroup(int groupId) const;
    int successorOf(int index) const;
    int mostRecentTabId(int groupId) const;
    int groupRowFor(int index) const;
    // Marks the tab in front as the most recent one, and tells the cover.
    void stampActive();
    void notifyRow(int index, Role role);
    void persist(const Tab &tab);
    void discardThumbnail(const QString &path) const;

    TabPersistence *m_persistence;
    QString m_thumbnailDirectory;
    QList<Tab> m_tabs;
    QList<TabGroup> m_groups;
    QList<int> m_awaitingFirstUrl;
    GroupTabModel *m_groupTabs;
    TabGroupModel *m_groupModel;
    ClosedTabModel *m_closedTabs;
    QSet<int> m_liveIds;
    int m_liveLimit = 0;
    int m_activeTabId = 0;
    int m_currentGroupId = 0;
    int m_nextTabId = 1;
    int m_nextGroupId = 1;
    // Counts activations rather than milliseconds: the order is all anyone reads, and a
    // counter cannot be turned around by a clock that steps backwards.
    qint64 m_activationClock = 0;
    int m_thumbnailCounter = 0;
};

} // namespace Salama
