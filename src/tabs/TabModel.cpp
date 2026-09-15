// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#include "TabModel.h"

#include "TabPersistence.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QtDebug>
#include <algorithm>
#include <utility>

namespace Tuuli {

TabModel::TabModel(TabPersistence *persistence, QString thumbnailDirectory, QObject *parent)
    : QAbstractListModel(parent)
    , m_persistence(persistence)
    , m_thumbnailDirectory(std::move(thumbnailDirectory))
{
    load();
}

void TabModel::load()
{
    if (m_persistence == nullptr) {
        return;
    }
    m_tabs = m_persistence->loadTabs();
    for (const Tab &tab : m_tabs) {
        m_nextTabId = std::max(m_nextTabId, tab.id + 1);
    }
    // A preview whose file went away shows as nothing rather than as a broken image.
    for (Tab &tab : m_tabs) {
        if (!tab.thumbnail.isEmpty() && !QFile::exists(tab.thumbnail)) {
            tab.thumbnail.clear();
        }
    }

    if (m_tabs.isEmpty()) {
        return;
    }
    const int storedActive = m_persistence->loadActiveTabId();
    m_activeTabId = indexOf(storedActive) >= 0 ? storedActive : m_tabs.first().id;
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
    case PrivateRole:
        return tab.isPrivate;
    case ActiveRole:
        return tab.id == m_activeTabId;
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
        {PrivateRole, QByteArrayLiteral("privateTab")},
        {ActiveRole, QByteArrayLiteral("activeTab")},
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

bool TabModel::activeIsPrivate() const
{
    const int index = activeTabIndex();
    return index >= 0 && m_tabs.at(index).isPrivate;
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

int TabModel::newTab(const QString &url, bool isPrivate)
{
    if (isExternalUrl(url)) {
        return 0;
    }

    Tab tab;
    tab.id = m_nextTabId++;
    tab.url = url;
    tab.isPrivate = isPrivate;

    const int index = m_tabs.count();
    beginInsertRows(QModelIndex(), index, index);
    m_tabs.append(tab);
    endInsertRows();
    m_awaitingFirstUrl.append(tab.id);

    if (m_persistence != nullptr) {
        m_persistence->insertTab(tab);
    }

    emit countChanged();
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

void TabModel::moveTab(int from, int to)
{
    const int last = m_tabs.count() - 1;
    if (from == to || from < 0 || from > last || to < 0 || to > last) {
        return;
    }
    // beginMoveRows wants the row the block lands *before*, which is one past the
    // destination when moving down the list.
    const int destination = to > from ? to + 1 : to;
    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), destination)) {
        return;
    }
    m_tabs.move(from, to);
    endMoveRows();

    if (m_persistence != nullptr) {
        m_persistence->saveOrder(m_tabs);
    }
    // activeTabIndex is a position, and positions have just changed.
    emit activeTabChanged();
}

void TabModel::closeTab(int index)
{
    if (index < 0 || index >= m_tabs.count()) {
        return;
    }
    const Tab closing = m_tabs.at(index);
    const bool closingActive = closing.id == m_activeTabId;
    discardThumbnail(closing.thumbnail);

    beginRemoveRows(QModelIndex(), index, index);
    m_tabs.removeAt(index);
    endRemoveRows();
    m_awaitingFirstUrl.removeAll(closing.id);

    if (m_persistence != nullptr) {
        m_persistence->removeTab(closing.id);
    }

    if (closingActive) {
        m_activeTabId = 0;
        if (m_tabs.isEmpty()) {
            if (m_persistence != nullptr) {
                m_persistence->setActiveTabId(0);
            }
            emit activeTabChanged();
            emit activeTabDataChanged();
        } else {
            // The tab before the closed one becomes active, or the first if none precedes it.
            activateTab(index - 1);
        }
    }

    // Last: listeners may open a replacement tab from here, which re-enters this model.
    emit tabClosed(closing.id);
    emit countChanged();
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
    if (!tab.isPrivate) {
        emit visited(url);
    }
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
    if (!tab.isPrivate) {
        emit titleUpdated(tab.url, title);
    }
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
    if (!tab.isPrivate) {
        emit faviconUpdated(tab.url, favicon);
    }
}

QString TabModel::thumbnailPath(int tabId)
{
    const int index = indexOf(tabId);
    if (m_thumbnailDirectory.isEmpty() || index < 0 || m_tabs.at(index).isPrivate) {
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
    if (index < 0 || m_tabs.at(index).isPrivate || m_tabs.at(index).thumbnail == path) {
        return;
    }
    Tab &tab = m_tabs[index];
    discardThumbnail(tab.thumbnail);
    tab.thumbnail = path;
    notifyRow(index, ThumbnailRole);
    persist(tab);
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

void TabModel::setActiveTab(int tabId)
{
    if (tabId == m_activeTabId) {
        return;
    }
    const int oldIndex = indexOf(m_activeTabId);
    m_activeTabId = tabId;
    if (oldIndex >= 0) {
        notifyRow(oldIndex, ActiveRole);
    }
    const int newIndex = indexOf(tabId);
    if (newIndex >= 0) {
        notifyRow(newIndex, ActiveRole);
    }
    if (m_persistence != nullptr) {
        m_persistence->setActiveTabId(tabId);
    }
    emit activeTabChanged();
    emit activeTabDataChanged();
}

void TabModel::notifyRow(int index, Role role)
{
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, QVector<int>{role});
}

void TabModel::persist(const Tab &tab)
{
    if (m_persistence != nullptr) {
        m_persistence->updateTab(tab);
    }
}

} // namespace Tuuli
