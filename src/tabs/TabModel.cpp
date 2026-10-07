// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "TabModel.h"

#include "ClosedTabModel.h"
#include "GroupTabModel.h"
#include "TabGroupModel.h"
#include "TabPersistence.h"
#include "ThumbnailWriter.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
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
    , m_thumbnailWriter(new ThumbnailWriter(this))
{
    load();
    ensureGroups();
    m_liveIds = liveSet();
    connect(this, &TabModel::activeTabChanged, this, &TabModel::activeMediaChanged);
    connect(this, &TabModel::recentTabsChanged, m_groupModel,
            [this]() { m_groupModel->changedAll(TabGroupModel::Role::Previews); });
    connect(m_thumbnailWriter, &ThumbnailWriter::written, this, &TabModel::thumbnailWritten);
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
    // Pre-schema-3 DB has no stamps; this gives first.
    stampActive();
}

// Missing group created unnamed, not tab moved: pre-schema-4 DB has all tabs in group 1, no
// group table.
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
    switch (static_cast<Role>(role)) {
    case Role::TabId:
        return tab.id;
    case Role::Url:
        return tab.url;
    case Role::Title:
        return tab.title;
    case Role::Favicon:
        return tab.favicon;
    case Role::Thumbnail:
        return tab.thumbnail;
    case Role::Active:
        return tab.id == m_activeTabId;
    case Role::Group:
        return tab.groupId;
    case Role::Live:
        return m_liveIds.contains(tab.id);
    case Role::Media:
        return shownMediaState(tab.id);
    case Role::Muted:
        return m_muted.contains(tab.id);
    default:
        return {};
    }
}

QHash<int, QByteArray> TabModel::roleNames() const
{
    return {
        {roleId(Role::TabId), QByteArrayLiteral("tabId")},
        {roleId(Role::Url), QByteArrayLiteral("url")},
        {roleId(Role::Title), QByteArrayLiteral("title")},
        {roleId(Role::Favicon), QByteArrayLiteral("favicon")},
        {roleId(Role::Thumbnail), QByteArrayLiteral("thumbnail")},
        {roleId(Role::Active), QByteArrayLiteral("activeTab")},
        {roleId(Role::Group), QByteArrayLiteral("groupId")},
        {roleId(Role::Live), QByteArrayLiteral("liveTab")},
        {roleId(Role::Media), QByteArrayLiteral("mediaState")},
        {roleId(Role::Muted), QByteArrayLiteral("muted")},
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

QString TabModel::activeMediaTitle() const
{
    return mediaMetadata(m_activeTabId).title;
}

QString TabModel::activeMediaArtist() const
{
    return mediaMetadata(m_activeTabId).artist;
}

QString TabModel::activeMediaArtwork() const
{
    return mediaMetadata(m_activeTabId).artwork;
}

const QList<Tab> &TabModel::tabs() const
{
    return m_tabs;
}

bool TabModel::isExternalUrl(const QString &url)
{
    const QString scheme = QUrl(url, QUrl::TolerantMode).scheme();
    return scheme == QLatin1String("tel") || scheme == QLatin1String("sms") ||
           scheme == QLatin1String("mailto") || scheme == QLatin1String("geo");
}

int TabModel::newTab(const QString &url)
{
    const int tabId = insertTab(url, QString());
    if (tabId != 0) {
        setActiveTab(tabId);
    }
    return tabId;
}

int TabModel::newTabBehind(const QString &url, const QString &title)
{
    return url.isEmpty() ? 0 : insertTab(url, title);
}

int TabModel::insertTab(const QString &url, const QString &title)
{
    if (isExternalUrl(url)) {
        return 0;
    }

    Tab tab;
    tab.id = m_nextTabId++;
    tab.url = url;
    tab.title = title;
    tab.groupId = m_currentGroupId;

    const int index = m_tabs.count();
    beginInsertRows(QModelIndex(), index, index);
    m_tabs.append(tab);
    endInsertRows();
    m_awaitingFirstUrl.append(tab.id);
    if (tab.groupId == m_currentGroupId) {
        m_groupTabs->append(tab.id);
    }
    m_groupModel->changed(groupIndexOf(tab.groupId), TabGroupModel::Role::TabCount);

    if (m_persistence != nullptr) {
        m_persistence->insertTab(tab);
    }

    emit countChanged();
    emit recentTabsChanged();
    emit tabAdded(tab.id);
    return tab.id;
}

int TabModel::newTabInDefaultGroup(const QString &url)
{
    if (isExternalUrl(url)) {
        return 0;
    }
    setCurrentGroupId(defaultGroupId());
    return newTab(url);
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
    // beginMoveRows wants row block lands *before*: dest + 1 when moving down.
    const int destination = to > from ? to + 1 : to;
    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), destination)) {
        return;
    }
    m_tabs.move(from, to);
    endMoveRows();
    if (groupRow >= 0) {
        m_groupTabs->moveTabRow(groupRow, groupRowFor(to));
    }

    if (m_persistence != nullptr) {
        m_persistence->saveOrder(m_tabs);
    }
    emit activeTabChanged();
}

// Previous in group, else next, else most recent anywhere. 0 if last tab.
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
    m_metadata.remove(closing.id);
    m_groupTabs->remove(closing.id);
    m_groupModel->changed(groupIndexOf(closing.groupId), TabGroupModel::Role::TabCount);

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
    refreshLive();
    m_closedTabs->record(closing);

    // Last: listeners may open replacement, re-entering model.
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
    m_metadata.clear();
    m_groupTabs->reset(QList<int>());
    m_groupModel->changedAll(TabGroupModel::Role::TabCount);
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

QString TabModel::groupNameOf(int tabId) const
{
    const int index = indexOf(tabId);
    if (index < 0) {
        return {};
    }
    const int group = groupIndexOf(m_tabs.at(index).groupId);
    return group < 0 ? QString() : m_groups.at(group).name;
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
    const bool leavesStartPage = tab.url.isEmpty();
    tab.url = url;
    notifyRow(index, Role::Url);
    persist(tab);
    if (tab.id == m_activeTabId) {
        emit activeTabDataChanged();
    }
    if (leavesStartPage) {
        refreshLive();
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
    notifyRow(index, Role::Title);
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
    notifyRow(index, Role::Favicon);
    persist(tab);
    if (tab.id == m_activeTabId) {
        emit activeTabDataChanged();
    }
    emit faviconUpdated(tab.url, favicon);
}

void TabModel::showStartPage(int tabId)
{
    const int index = indexOf(tabId);
    if (index < 0 || m_tabs.at(index).url.isEmpty()) {
        return;
    }
    Tab &tab = m_tabs[index];
    m_pendingThumbnails.remove(tabId);
    tab.url.clear();
    tab.title.clear();
    tab.favicon.clear();
    discardThumbnail(tab.thumbnail);
    tab.thumbnail.clear();
    for (const Role role : {Role::Url, Role::Title, Role::Favicon, Role::Thumbnail}) {
        notifyRow(index, role);
    }
    persist(tab);
    if (tab.id == m_activeTabId) {
        emit activeTabDataChanged();
    }
    setMediaState(tabId, NoMedia);
    refreshLive();
    emit recentTabsChanged();
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
    // Counter + clock: two grabs in same ms differ.
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
    notifyRow(index, Role::Thumbnail);
    persist(tab);
    emit recentTabsChanged();
}

bool TabModel::storeThumbnail(int tabId, const QVariant &image)
{
    const QImage picture = image.value<QImage>();
    if (picture.isNull()) {
        return false;
    }
    const QString path = thumbnailPath(tabId);
    if (path.isEmpty()) {
        return false;
    }
    m_pendingThumbnails.insert(tabId, path);
    m_thumbnailWriter->write(tabId, picture, path);
    return true;
}

void TabModel::thumbnailWritten(int tabId, const QString &path, bool saved)
{
    const bool newest = m_pendingThumbnails.value(tabId) == path;
    if (newest) {
        m_pendingThumbnails.remove(tabId);
    }
    if (saved && newest && indexOf(tabId) >= 0) {
        updateThumbnail(tabId, path);
    } else {
        discardThumbnail(path);
    }
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
    // Late answer for unloaded page: ignore.
    if (index < 0 || mediaState(tabId) == state ||
        (state != NoMedia && !m_liveIds.contains(tabId))) {
        return;
    }
    if (state == NoMedia) {
        m_media.remove(tabId);
        m_metadata.remove(tabId);
    } else {
        m_media.insert(tabId, state);
    }
    notifyRow(index, Role::Media);
    if (tabId == m_activeTabId) {
        emit activeMediaChanged();
    }
}

TabModel::MediaMetadata TabModel::mediaMetadata(int tabId) const
{
    return m_metadata.value(tabId);
}

void TabModel::setMediaMetadata(int tabId, const MediaMetadata &metadata)
{
    if (mediaState(tabId) == NoMedia || mediaMetadata(tabId) == metadata) {
        return;
    }
    m_metadata.insert(tabId, metadata);
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
    notifyRow(index, Role::Muted);
    if (tabId == m_activeTabId) {
        emit activeMediaChanged();
    }
}

void TabModel::discardThumbnail(const QString &path) const
{
    // Only deletes own files: stray DB value can't delete elsewhere.
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
    m_groupModel->changed(oldIndex, TabGroupModel::Role::Current);
    m_groupModel->changed(groupIndexOf(groupId), TabGroupModel::Role::Current);
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
    const int row = m_groups.count();
    m_groups.append(group);
    m_groupModel->inserted(row);
    if (m_persistence != nullptr) {
        m_persistence->insertGroup(group);
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
    m_groupModel->changed(index, TabGroupModel::Role::Name);
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
    // Switch group first: closing may close active; replacement must not open in dying group.
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
    notifyRow(index, Role::Group);
    persist(tab);
    if (oldGroup == m_currentGroupId) {
        m_groupTabs->remove(tab.id);
    } else if (groupId == m_currentGroupId) {
        m_groupTabs->append(tab.id);
    }
    for (const int group : {oldGroup, groupId}) {
        m_groupModel->changed(groupIndexOf(group), TabGroupModel::Role::TabCount);
        m_groupModel->changed(groupIndexOf(group), TabGroupModel::Role::Previews);
    }
    emit groupsChanged();
    // Active tab always in current group.
    if (tab.id == m_activeTabId) {
        setCurrentGroupId(groupId);
    }
    return true;
}

bool TabModel::ungroup(int groupId)
{
    const int index = groupIndexOf(groupId);
    const int home = defaultGroupId();
    if (index < 0 || groupId == home) {
        return false;
    }
    for (int i = 0; i < m_tabs.count(); ++i) {
        Tab &tab = m_tabs[i];
        if (tab.groupId != groupId) {
            continue;
        }
        tab.groupId = home;
        notifyRow(i, Role::Group);
        persist(tab);
        if (home == m_currentGroupId) {
            m_groupTabs->append(tab.id);
        }
    }
    m_groupModel->changed(groupIndexOf(home), TabGroupModel::Role::TabCount);
    m_groupModel->changed(groupIndexOf(home), TabGroupModel::Role::Previews);
    if (groupId == m_currentGroupId) {
        setCurrentGroupId(home);
    }
    m_groups.removeAt(index);
    m_groupModel->removed(index);
    if (m_persistence != nullptr) {
        m_persistence->removeGroup(groupId);
    }
    emit groupsChanged();
    emit currentGroupChanged();
    return true;
}

bool TabModel::moveGroup(int from, int to)
{
    const int last = m_groups.count() - 1;
    if (from == to || from < 1 || from > last || to < 1 || to > last) {
        return false;
    }
    const int currentIndex = currentGroupIndex();
    m_groups.move(from, to);
    m_groupModel->moved(from, to);
    if (m_persistence != nullptr) {
        m_persistence->saveGroupOrder(m_groups);
    }
    emit groupsChanged();
    if (currentGroupIndex() != currentIndex) {
        emit currentGroupChanged();
    }
    return true;
}

QStringList TabModel::groupThumbnails(int groupId, int limit) const
{
    // Sort copy: model order is grid/persisted order. stable_sort keeps never-fronted tabs in grid
    // order.
    QList<const Tab *> ordered;
    for (const Tab &tab : m_tabs) {
        if (tab.groupId == groupId) {
            ordered.append(&tab);
        }
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const Tab *one, const Tab *other) {
        return one->lastActive > other->lastActive;
    });

    QStringList thumbnails;
    for (int i = 0; i < ordered.count() && i < limit; ++i) {
        thumbnails.append(ordered.at(i)->thumbnail);
    }
    return thumbnails;
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
    // Start-page tab: live but not counted, holds no page.
    QSet<int> live;
    int pages = 0;
    for (const Tab *tab : ordered) {
        if (tab->url.isEmpty()) {
            live.insert(tab->id);
            continue;
        }
        if (m_liveLimit == 0 || pages < m_liveLimit || tab->id == m_activeTabId) {
            live.insert(tab->id);
        }
        ++pages;
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
            notifyRow(i, Role::Live);
        }
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

// Kept apart from setCurrentGroupId()/setActiveTab(): calling each other cycled.
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
    for (const int index : {oldIndex, indexOf(tabId)}) {
        if (index >= 0) {
            notifyRow(index, Role::Active);
            if (m_media.value(m_tabs.at(index).id, NoMedia) == MediaPlaying) {
                notifyRow(index, Role::Media);
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
    emit dataChanged(modelIndex, modelIndex, QVector<int>{roleId(role)});
    m_groupTabs->changed(m_tabs.at(index).id, roleId(role));
}

void TabModel::persist(const Tab &tab) const
{
    if (m_persistence != nullptr) {
        m_persistence->updateTab(tab);
    }
}

} // namespace Salama
