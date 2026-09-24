// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "TabModel.h"

#include "ClosedTabModel.h"
#include "GroupTabModel.h"
#include "TabGroupModel.h"
#include "TabPersistence.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QtDebug>
#include <algorithm>
#include <utility>

namespace Salama {

TabModel::TabModel(TabPersistence *persistence, QString thumbnailDirectory, QObject *parent)
    : QAbstractListModel(parent)
    , m_persistence(persistence)
    , m_thumbnailDirectory(std::move(thumbnailDirectory))
    , m_groupTabs(new GroupTabModel(this))
    , m_groupModel(new TabGroupModel(this))
    , m_closedTabs(new ClosedTabModel(this, persistence))
{
    load();
    ensureGroups();
    m_liveIds = liveSet();
    // Another tab in front is other media in front.
    connect(this, &TabModel::activeTabChanged, this, &TabModel::activeMediaChanged);
}

void TabModel::load()
{
    if (m_persistence == nullptr) {
        return;
    }
    m_tabs = m_persistence->loadTabs();
    m_groups = m_persistence->loadGroups();
    for (const Tab &tab : m_tabs) {
        m_nextTabId = std::max(m_nextTabId, tab.id + 1);
    }
    // A preview whose file went away shows as nothing rather than as a broken image.
    for (Tab &tab : m_tabs) {
        if (!tab.thumbnail.isEmpty() && !QFile::exists(tab.thumbnail)) {
            tab.thumbnail.clear();
        }
    }

    for (const Tab &tab : m_tabs) {
        m_activationClock = std::max(m_activationClock, tab.lastActive);
    }

    m_currentGroupId = m_persistence->loadCurrentGroupId();
    if (m_tabs.isEmpty()) {
        return;
    }
    const int storedActive = m_persistence->loadActiveTabId();
    m_activeTabId = indexOf(storedActive) >= 0 ? storedActive : m_tabs.first().id;
    // The restored tab is in front from here, whatever the database said about which
    // was in front last. A database written before schema 3 has no stamps at all, and
    // this is what gives the first one out.
    stampActive();
}

// Every tab is in a group that exists, there is at least one group, and the current
// group is among them. A group a stored tab names but no row describes is created
// unnamed rather than the tab moved: that is what a database from before schema 4
// looks like, where every tab says group 1 and no group table says anything.
void TabModel::ensureGroups()
{
    for (const TabGroup &group : m_groups) {
        m_nextGroupId = std::max(m_nextGroupId, group.id + 1);
    }
    for (Tab &tab : m_tabs) {
        if (tab.groupId <= 0) {
            tab.groupId = 1;
        }
        if (groupIndexOf(tab.groupId) < 0) {
            TabGroup group;
            group.id = tab.groupId;
            m_groups.append(group);
            m_nextGroupId = std::max(m_nextGroupId, group.id + 1);
            if (m_persistence != nullptr) {
                m_persistence->insertGroup(group);
            }
        }
    }
    if (m_groups.isEmpty()) {
        TabGroup group;
        group.id = m_nextGroupId++;
        m_groups.append(group);
        if (m_persistence != nullptr) {
            m_persistence->insertGroup(group);
        }
    }

    // The active tab's group first, then what was stored, then the first there is.
    const int activeIndex = indexOf(m_activeTabId);
    if (activeIndex >= 0) {
        m_currentGroupId = m_tabs.at(activeIndex).groupId;
    }
    if (groupIndexOf(m_currentGroupId) < 0) {
        m_currentGroupId = defaultGroupId();
    }
    QList<int> ids;
    for (const Tab &tab : m_tabs) {
        if (tab.groupId == m_currentGroupId) {
            ids.append(tab.id);
        }
    }
    m_groupTabs->reset(ids);
}

void TabModel::stampActive()
{
    const int index = indexOf(m_activeTabId);
    if (index < 0) {
        return;
    }
    Tab &tab = m_tabs[index];
    tab.lastActive = ++m_activationClock;
    persist(tab);
    emit recentTabsChanged();
}

int TabModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_tabs.count();
}

QVariant TabModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_tabs.count()) {
        return {};
    }
    const Tab &tab = m_tabs.at(index.row());
    switch (role) {
    case TabIdRole:
        return tab.id;
    case UrlRole:
        return tab.url;
    case TitleRole:
        return tab.title;
    case FaviconRole:
        return tab.favicon;
    case ThumbnailRole:
        return tab.thumbnail;
    case ActiveRole:
        return tab.id == m_activeTabId;
    case GroupRole:
        return tab.groupId;
    case LiveRole:
        return m_liveIds.contains(tab.id);
    case MediaRole:
        return shownMediaState(tab.id);
    case MutedRole:
        return m_muted.contains(tab.id);
    default:
        return {};
    }
}

QHash<int, QByteArray> TabModel::roleNames() const
{
    return {
        {TabIdRole, QByteArrayLiteral("tabId")},
        {UrlRole, QByteArrayLiteral("url")},
        {TitleRole, QByteArrayLiteral("title")},
        {FaviconRole, QByteArrayLiteral("favicon")},
        {ThumbnailRole, QByteArrayLiteral("thumbnail")},
        {ActiveRole, QByteArrayLiteral("activeTab")},
        {GroupRole, QByteArrayLiteral("groupId")},
        {LiveRole, QByteArrayLiteral("liveTab")},
        {MediaRole, QByteArrayLiteral("mediaState")},
        {MutedRole, QByteArrayLiteral("muted")},
    };
}

int TabModel::count() const
{
    return m_tabs.count();
}

int TabModel::activeTabIndex() const
{
    return indexOf(m_activeTabId);
}

int TabModel::activeTabId() const
{
    return m_activeTabId;
}

QString TabModel::activeUrl() const
{
    const int index = activeTabIndex();
    return index >= 0 ? m_tabs.at(index).url : QString();
}

QString TabModel::activeTitle() const
{
    const int index = activeTabIndex();
    return index >= 0 ? m_tabs.at(index).title : QString();
}

QString TabModel::activeFavicon() const
{
    const int index = activeTabIndex();
    return index >= 0 ? m_tabs.at(index).favicon : QString();
}

int TabModel::activeMediaState() const
{
    return shownMediaState(m_activeTabId);
}

bool TabModel::activeMuted() const
{
    return isMuted(m_activeTabId);
}

QStringList TabModel::recentThumbnails() const
{
    // Ordered on a copy of the ids: the model's own order is what the grid shows and
    // what is persisted, and the cover must not disturb either. std::stable_sort so
    // that tabs never yet in front -- restored ones, before they are opened -- keep the
    // order the grid puts them in rather than an arbitrary one.
    QList<const Tab *> ordered;
    ordered.reserve(m_tabs.count());
    for (const Tab &tab : m_tabs) {
        ordered.append(&tab);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const Tab *one, const Tab *other) {
        return one->lastActive > other->lastActive;
    });

    QStringList thumbnails;
    thumbnails.reserve(ordered.count());
    for (const Tab *tab : ordered) {
        thumbnails.append(tab->thumbnail);
    }
    return thumbnails;
}

const QList<Tab> &TabModel::tabs() const
{
    return m_tabs;
}

bool TabModel::isExternalUrl(const QString &url)
{
    // Schemes the engine hands to other applications; they never become tabs.
    const QString scheme = QUrl(url, QUrl::TolerantMode).scheme();
    return scheme == QLatin1String("tel") || scheme == QLatin1String("sms") ||
           scheme == QLatin1String("mailto") || scheme == QLatin1String("geo");
}

int TabModel::newTab(const QString &url)
{
    if (isExternalUrl(url)) {
        return 0;
    }

    Tab tab;
    tab.id = m_nextTabId++;
    tab.url = url;
    tab.groupId = m_currentGroupId;

    const int index = m_tabs.count();
    beginInsertRows(QModelIndex(), index, index);
    m_tabs.append(tab);
    endInsertRows();
    m_awaitingFirstUrl.append(tab.id);
    if (tab.groupId == m_currentGroupId) {
        m_groupTabs->append(tab.id);
    }
    m_groupModel->changed(groupIndexOf(tab.groupId), TabGroupModel::TabCountRole);

    if (m_persistence != nullptr) {
        m_persistence->insertTab(tab);
    }

    emit countChanged();
    emit recentTabsChanged();
    emit tabAdded(tab.id);
    setActiveTab(tab.id);
    return tab.id;
}

void TabModel::activateTab(int index)
{
    if (m_tabs.isEmpty()) {
        return;
    }
    const int bounded = std::min(std::max(index, 0), m_tabs.count() - 1);
    setActiveTab(m_tabs.at(bounded).id);
}

bool TabModel::activateTabById(int tabId)
{
    const int index = indexOf(tabId);
    if (index < 0) {
        return false;
    }
    setActiveTab(tabId);
    return true;
}

// How many tabs of the current group sit before this row: the row the grid would
// show the tab at, were it in that group.
int TabModel::groupRowFor(int index) const
{
    int row = 0;
    for (int i = 0; i < index && i < m_tabs.count(); ++i) {
        if (m_tabs.at(i).groupId == m_currentGroupId) {
            ++row;
        }
    }
    return row;
}

void TabModel::moveTab(int from, int to)
{
    const int last = m_tabs.count() - 1;
    if (from == to || from < 0 || from > last || to < 0 || to > last) {
        return;
    }
    const int groupRow = m_groupTabs->rowOf(m_tabs.at(from).id);
    // beginMoveRows wants the row the block lands *before*, which is one past the
    // destination when moving down the list.
    const int destination = to > from ? to + 1 : to;
    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), destination)) {
        return;
    }
    m_tabs.move(from, to);
    endMoveRows();
    // The grid's order is this order with the other groups' tabs left out, so a tab
    // of the current group lands in the grid where the tabs before it put it.
    if (groupRow >= 0) {
        m_groupTabs->moveTabRow(groupRow, groupRowFor(to));
    }

    if (m_persistence != nullptr) {
        m_persistence->saveOrder(m_tabs);
    }
    // activeTabIndex is a position, and positions have just changed.
    emit activeTabChanged();
}

// The tab to bring to the front when the one at this row closes: the one before it in
// its own group, else the one after, else the most recent tab in any group. Zero when
// it was the last tab there is.
int TabModel::successorOf(int index) const
{
    const Tab &closing = m_tabs.at(index);
    for (int i = index - 1; i >= 0; --i) {
        if (m_tabs.at(i).groupId == closing.groupId) {
            return m_tabs.at(i).id;
        }
    }
    for (int i = index + 1; i < m_tabs.count(); ++i) {
        if (m_tabs.at(i).groupId == closing.groupId) {
            return m_tabs.at(i).id;
        }
    }
    int successor = 0;
    qint64 latest = -1;
    for (int i = 0; i < m_tabs.count(); ++i) {
        if (i != index && m_tabs.at(i).lastActive > latest) {
            latest = m_tabs.at(i).lastActive;
            successor = m_tabs.at(i).id;
        }
    }
    return successor;
}

// The tab of this group that was in front last, or zero for an empty group.
int TabModel::mostRecentTabId(int groupId) const
{
    int recent = 0;
    qint64 latest = -1;
    for (const Tab &tab : m_tabs) {
        if (tab.groupId == groupId && tab.lastActive > latest) {
            latest = tab.lastActive;
            recent = tab.id;
        }
    }
    return recent;
}

void TabModel::closeTab(int index)
{
    if (index < 0 || index >= m_tabs.count()) {
        return;
    }
    const Tab closing = m_tabs.at(index);
    const bool closingActive = closing.id == m_activeTabId;
    const int successor = closingActive ? successorOf(index) : 0;
    discardThumbnail(closing.thumbnail);

    beginRemoveRows(QModelIndex(), index, index);
    m_tabs.removeAt(index);
    endRemoveRows();
    m_awaitingFirstUrl.removeAll(closing.id);
    m_media.remove(closing.id);
    m_muted.remove(closing.id);
    m_groupTabs->remove(closing.id);
    m_groupModel->changed(groupIndexOf(closing.groupId), TabGroupModel::TabCountRole);

    if (m_persistence != nullptr) {
        m_persistence->removeTab(closing.id);
    }

    if (closingActive) {
        m_activeTabId = 0;
        if (successor == 0) {
            if (m_persistence != nullptr) {
                m_persistence->setActiveTabId(0);
            }
            emit activeTabChanged();
            emit activeTabDataChanged();
        } else {
            setActiveTab(successor);
        }
    }
    // A tab gone may leave room for another to keep its page.
    refreshLive();
    m_closedTabs->record(closing);

    // Last: listeners may open a replacement tab from here, which re-enters this model.
    emit tabClosed(closing.id);
    emit countChanged();
    emit recentTabsChanged();
}

void TabModel::closeTabById(int tabId)
{
    closeTab(indexOf(tabId));
}

void TabModel::closeActiveTab()
{
    closeTab(activeTabIndex());
}

void TabModel::closeAllTabs()
{
    if (m_tabs.isEmpty()) {
        return;
    }
    const QList<Tab> closed = m_tabs;
    for (const Tab &tab : closed) {
        discardThumbnail(tab.thumbnail);
    }
    beginRemoveRows(QModelIndex(), 0, m_tabs.count() - 1);
    m_tabs.clear();
    endRemoveRows();
    m_awaitingFirstUrl.clear();
    m_activeTabId = 0;
    m_liveIds.clear();
    m_media.clear();
    m_muted.clear();
    m_groupTabs->reset(QList<int>());
    m_groupModel->changedAll(TabGroupModel::TabCountRole);
    for (const Tab &tab : closed) {
        m_closedTabs->record(tab);
    }

    if (m_persistence != nullptr) {
        m_persistence->removeAllTabs();
        m_persistence->setActiveTabId(0);
    }

    emit activeTabChanged();
    emit activeTabDataChanged();
    for (const Tab &tab : closed) {
        emit tabClosed(tab.id);
    }
    emit countChanged();
    emit recentTabsChanged();
}

int TabModel::indexOf(int tabId) const
{
    for (int i = 0; i < m_tabs.count(); ++i) {
        if (m_tabs.at(i).id == tabId) {
            return i;
        }
    }
    return -1;
}

int TabModel::tabIdForUrl(const QString &url) const
{
    if (url.isEmpty()) {
        return 0;
    }
    int found = 0;
    qint64 latest = -1;
    for (const Tab &tab : m_tabs) {
        if (tab.url == url && tab.lastActive > latest) {
            latest = tab.lastActive;
            found = tab.id;
        }
    }
    return found;
}

void TabModel::updateUrl(int tabId, const QString &url)
{
    if (url.isEmpty() || isExternalUrl(url)) {
        return;
    }
    const int index = indexOf(tabId);
    if (index < 0) {
        return;
    }
    Tab &tab = m_tabs[index];
    const bool firstReport = m_awaitingFirstUrl.removeAll(tabId) > 0;
    if (!firstReport && tab.url == url) {
        return;
    }
    tab.url = url;
    notifyRow(index, UrlRole);
    persist(tab);
    if (tab.id == m_activeTabId) {
        emit activeTabDataChanged();
    }
    emit visited(url);
}

void TabModel::updateTitle(int tabId, const QString &title)
{
    const int index = indexOf(tabId);
    if (index < 0 || m_tabs.at(index).title == title) {
        return;
    }
    Tab &tab = m_tabs[index];
    tab.title = title;
    notifyRow(index, TitleRole);
    persist(tab);
    if (tab.id == m_activeTabId) {
        emit activeTabDataChanged();
    }
    emit titleUpdated(tab.url, title);
}

void TabModel::updateFavicon(int tabId, const QString &favicon)
{
    const int index = indexOf(tabId);
    if (index < 0 || m_tabs.at(index).favicon == favicon) {
        return;
    }
    Tab &tab = m_tabs[index];
    tab.favicon = favicon;
    notifyRow(index, FaviconRole);
    persist(tab);
    if (tab.id == m_activeTabId) {
        emit activeTabDataChanged();
    }
    emit faviconUpdated(tab.url, favicon);
}

QString TabModel::thumbnailPath(int tabId)
{
    const int index = indexOf(tabId);
    if (m_thumbnailDirectory.isEmpty() || index < 0) {
        return {};
    }
    QDir dir(m_thumbnailDirectory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qWarning() << "TabModel: cannot create preview directory" << m_thumbnailDirectory;
        return {};
    }
    // Unique per capture: a counter alongside the clock, so two grabs within the same
    // millisecond still differ.
    return dir.absoluteFilePath(QStringLiteral("tab-%1-%2-%3.png")
                                    .arg(tabId)
                                    .arg(QDateTime::currentMSecsSinceEpoch())
                                    .arg(++m_thumbnailCounter));
}

void TabModel::updateThumbnail(int tabId, const QString &path)
{
    const int index = indexOf(tabId);
    if (index < 0 || m_tabs.at(index).thumbnail == path) {
        return;
    }
    Tab &tab = m_tabs[index];
    discardThumbnail(tab.thumbnail);
    tab.thumbnail = path;
    notifyRow(index, ThumbnailRole);
    persist(tab);
    emit recentTabsChanged();
}

TabModel::MediaState TabModel::mediaState(int tabId) const
{
    return m_media.value(tabId, NoMedia);
}

TabModel::MediaState TabModel::shownMediaState(int tabId) const
{
    const MediaState state = mediaState(tabId);
    return state == MediaPlaying && tabId != m_activeTabId ? MediaPaused : state;
}

void TabModel::setMediaState(int tabId, MediaState state)
{
    const int index = indexOf(tabId);
    // An answer that arrives after its page was given up is about a page that is gone.
    if (index < 0 || mediaState(tabId) == state ||
        (state != NoMedia && !m_liveIds.contains(tabId))) {
        return;
    }
    if (state == NoMedia) {
        m_media.remove(tabId);
    } else {
        m_media.insert(tabId, state);
    }
    notifyRow(index, MediaRole);
    if (tabId == m_activeTabId) {
        emit activeMediaChanged();
    }
}

bool TabModel::isMuted(int tabId) const
{
    return m_muted.contains(tabId);
}

void TabModel::setMuted(int tabId, bool muted)
{
    const int index = indexOf(tabId);
    if (index < 0 || isMuted(tabId) == muted) {
        return;
    }
    if (muted) {
        m_muted.insert(tabId);
    } else {
        m_muted.remove(tabId);
    }
    notifyRow(index, MutedRole);
    if (tabId == m_activeTabId) {
        emit activeMediaChanged();
    }
}

void TabModel::discardThumbnail(const QString &path) const
{
    // Only ever removes what this model handed out, so a stray value in the database
    // cannot turn into a delete somewhere else.
    if (path.isEmpty() || m_thumbnailDirectory.isEmpty()) {
        return;
    }
    if (QFileInfo(path).absolutePath() != QDir(m_thumbnailDirectory).absolutePath()) {
        qWarning() << "TabModel: refusing to remove a preview outside" << m_thumbnailDirectory;
        return;
    }
    QFile::remove(path);
}

const QList<TabGroup> &TabModel::groups() const
{
    return m_groups;
}

int TabModel::groupIndexOf(int groupId) const
{
    for (int i = 0; i < m_groups.count(); ++i) {
        if (m_groups.at(i).id == groupId) {
            return i;
        }
    }
    return -1;
}

int TabModel::tabCountInGroup(int groupId) const
{
    int count = 0;
    for (const Tab &tab : m_tabs) {
        if (tab.groupId == groupId) {
            ++count;
        }
    }
    return count;
}

int TabModel::defaultGroupId() const
{
    return m_groups.isEmpty() ? 0 : m_groups.first().id;
}

int TabModel::currentGroupId() const
{
    return m_currentGroupId;
}

int TabModel::currentGroupIndex() const
{
    return groupIndexOf(m_currentGroupId);
}

void TabModel::setCurrentGroupId(int groupId)
{
    if (groupId == m_currentGroupId || groupIndexOf(groupId) < 0) {
        return;
    }
    applyCurrentGroup(groupId);
    // Each group remembers which of its tabs was in front, the way Safari's do: what
    // the page shows once the grid is put away is the group that was chosen, not the
    // one it was opened from. An empty group leaves the page as it is.
    const int recent = mostRecentTabId(groupId);
    if (recent != 0 && recent != m_activeTabId) {
        applyActiveTab(recent);
    }
}

void TabModel::applyCurrentGroup(int groupId)
{
    const int oldIndex = groupIndexOf(m_currentGroupId);
    m_currentGroupId = groupId;
    QList<int> ids;
    for (const Tab &tab : m_tabs) {
        if (tab.groupId == groupId) {
            ids.append(tab.id);
        }
    }
    m_groupTabs->reset(ids);
    m_groupModel->changed(oldIndex, TabGroupModel::CurrentRole);
    m_groupModel->changed(groupIndexOf(groupId), TabGroupModel::CurrentRole);
    if (m_persistence != nullptr) {
        m_persistence->setCurrentGroupId(groupId);
    }
    emit currentGroupChanged();
}

int TabModel::addGroup(const QString &name)
{
    TabGroup group;
    group.id = m_nextGroupId++;
    group.name = name.trimmed();
    // Last: the default group keeps the front of the strip.
    const int row = m_groups.count();
    m_groups.append(group);
    m_groupModel->inserted(row);
    if (m_persistence != nullptr) {
        m_persistence->insertGroup(group);
        // Positions are given out in insertion order; renumbered so a restart keeps
        // the order the strip shows.
        m_persistence->saveGroupOrder(m_groups);
    }
    emit groupsChanged();
    setCurrentGroupId(group.id);
    return group.id;
}

void TabModel::renameGroup(int groupId, const QString &name)
{
    const int index = groupIndexOf(groupId);
    if (index < 0 || groupId == defaultGroupId() || m_groups.at(index).name == name.trimmed()) {
        return;
    }
    m_groups[index].name = name.trimmed();
    m_groupModel->changed(index, TabGroupModel::NameRole);
    if (m_persistence != nullptr) {
        m_persistence->updateGroup(m_groups.at(index));
    }
    emit groupsChanged();
}

bool TabModel::removeGroup(int groupId)
{
    const int index = groupIndexOf(groupId);
    if (index < 0 || groupId == defaultGroupId()) {
        return false;
    }
    // Current moves to the group before it first: closing the group's tabs can close
    // the active one, and whoever answers that by opening a replacement must not open
    // it in the group that is going.
    if (groupId == m_currentGroupId) {
        setCurrentGroupId(m_groups.at(index - 1).id);
    }
    for (int i = m_tabs.count() - 1; i >= 0; --i) {
        if (m_tabs.at(i).groupId == groupId) {
            closeTab(i);
        }
    }
    m_groups.removeAt(index);
    m_groupModel->removed(index);
    if (m_persistence != nullptr) {
        m_persistence->removeGroup(groupId);
    }
    emit groupsChanged();
    // The current group's row may have moved up.
    emit currentGroupChanged();
    return true;
}

bool TabModel::moveTabToGroup(int tabId, int groupId)
{
    const int index = indexOf(tabId);
    if (index < 0 || groupIndexOf(groupId) < 0) {
        return false;
    }
    Tab &tab = m_tabs[index];
    if (tab.groupId == groupId) {
        return true;
    }
    const int oldGroup = tab.groupId;
    tab.groupId = groupId;
    notifyRow(index, GroupRole);
    persist(tab);
    if (oldGroup == m_currentGroupId) {
        m_groupTabs->remove(tab.id);
    } else if (groupId == m_currentGroupId) {
        m_groupTabs->append(tab.id);
    }
    m_groupModel->changed(groupIndexOf(oldGroup), TabGroupModel::TabCountRole);
    m_groupModel->changed(groupIndexOf(groupId), TabGroupModel::TabCountRole);
    emit groupsChanged();
    // The active tab is in the current group, always: a tab moved away takes the
    // current group with it.
    if (tab.id == m_activeTabId) {
        setCurrentGroupId(groupId);
    }
    return true;
}

int TabModel::liveTabLimit() const
{
    return m_liveLimit;
}

void TabModel::setLiveTabLimit(int limit)
{
    const int bounded = std::max(0, limit);
    if (bounded == m_liveLimit) {
        return;
    }
    m_liveLimit = bounded;
    refreshLive();
}

// The tab in front and the ones read most recently before it keep their pages; the
// rest are unloaded by the view and reloaded when they come to the front again, the
// way Jolla's browser keeps five (docs/DECISIONS/0016-five-live-pages.md). Ordered
// as the cover orders them, by the activation stamp, stable over the list.
QSet<int> TabModel::liveSet() const
{
    QList<const Tab *> ordered;
    ordered.reserve(m_tabs.count());
    for (const Tab &tab : m_tabs) {
        ordered.append(&tab);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const Tab *one, const Tab *other) {
        return one->lastActive > other->lastActive;
    });
    QSet<int> live;
    for (int i = 0; i < ordered.count(); ++i) {
        if (m_liveLimit == 0 || i < m_liveLimit || ordered.at(i)->id == m_activeTabId) {
            live.insert(ordered.at(i)->id);
        }
    }
    return live;
}

void TabModel::refreshLive()
{
    const QSet<int> live = liveSet();
    if (live == m_liveIds) {
        return;
    }
    const QSet<int> before = m_liveIds;
    m_liveIds = live;
    for (int i = 0; i < m_tabs.count(); ++i) {
        const int id = m_tabs.at(i).id;
        if (before.contains(id) != live.contains(id)) {
            notifyRow(i, LiveRole);
        }
        // Its view goes, and whatever it was playing with it.
        if (!live.contains(id)) {
            setMediaState(id, NoMedia);
        }
    }
}

GroupTabModel *TabModel::groupTabs() const
{
    return m_groupTabs;
}

ClosedTabModel *TabModel::closedTabs() const
{
    return m_closedTabs;
}

TabGroupModel *TabModel::groupModel() const
{
    return m_groupModel;
}

// The two halves below are kept apart from setCurrentGroupId() and setActiveTab() so
// that neither calls the other: the group follows the tab and the tab follows the
// group, and done as one pair of functions that was a cycle.
void TabModel::setActiveTab(int tabId)
{
    if (tabId == m_activeTabId) {
        return;
    }
    applyActiveTab(tabId);
    const int index = indexOf(tabId);
    if (index >= 0 && m_tabs.at(index).groupId != m_currentGroupId) {
        applyCurrentGroup(m_tabs.at(index).groupId);
    }
}

void TabModel::applyActiveTab(int tabId)
{
    if (m_activeTabId != 0 && m_activeTabId != tabId) {
        emit activeTabLeaving(m_activeTabId);
    }
    const int oldIndex = indexOf(m_activeTabId);
    m_activeTabId = tabId;
    // Media shown as held behind the front, or no longer (shownMediaState()).
    for (const int index : {oldIndex, indexOf(tabId)}) {
        if (index >= 0) {
            notifyRow(index, ActiveRole);
            if (m_media.value(m_tabs.at(index).id, NoMedia) == MediaPlaying) {
                notifyRow(index, MediaRole);
            }
        }
    }
    if (m_persistence != nullptr) {
        m_persistence->setActiveTabId(tabId);
    }
    stampActive();
    refreshLive();
    emit activeTabChanged();
    emit activeTabDataChanged();
}

void TabModel::notifyRow(int index, Role role)
{
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, QVector<int>{role});
    m_groupTabs->changed(m_tabs.at(index).id, role);
}

void TabModel::persist(const Tab &tab)
{
    if (m_persistence != nullptr) {
        m_persistence->updateTab(tab);
    }
}

} // namespace Salama
