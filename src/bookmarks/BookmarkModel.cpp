// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "BookmarkModel.h"

#include "search/SearchWords.h"
#include "storage/Storage.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

namespace Salama {

namespace {

bool run(QSqlQuery &query)
{
    if (!query.exec()) {
        qWarning() << "BookmarkModel:" << query.lastError().text() << query.lastQuery();
        return false;
    }
    return true;
}

// The helpers below read the list rather than the model, so that they are this file's
// own rather than more members of a class Qt's model interface already makes long.

// What a bookmark is called where it is shown: its title, or its address without one.
QString shownTitle(const BookmarkModel::Bookmark &bookmark)
{
    return bookmark.title.isEmpty() ? bookmark.url : bookmark.title;
}

int indexOfId(const QList<BookmarkModel::Bookmark> &bookmarks, int id)
{
    for (int i = 0; i < bookmarks.count(); ++i) {
        if (bookmarks.at(i).id == id) {
            return i;
        }
    }
    return -1;
}

int indexOfUrl(const QList<BookmarkModel::Bookmark> &bookmarks, const QString &url)
{
    for (int i = 0; i < bookmarks.count(); ++i) {
        if (bookmarks.at(i).url == url) {
            return i;
        }
    }
    return -1;
}

} // namespace

BookmarkModel::BookmarkModel(const Storage &storage, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(storage.database())
{
    reload();
}

int BookmarkModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_bookmarks.count();
}

QVariant BookmarkModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_bookmarks.count()) {
        return {};
    }
    const Bookmark &bookmark = m_bookmarks.at(index.row());
    switch (static_cast<Role>(role)) {
    case Role::BookmarkId:
        return bookmark.id;
    case Role::Url:
        return bookmark.url;
    case Role::Title:
        return shownTitle(bookmark);
    case Role::Favicon:
        return bookmark.favicon;
    default:
        return {};
    }
}

QHash<int, QByteArray> BookmarkModel::roleNames() const
{
    return {
        {roleId(Role::BookmarkId), QByteArrayLiteral("bookmarkId")},
        {roleId(Role::Url), QByteArrayLiteral("url")},
        {roleId(Role::Title), QByteArrayLiteral("title")},
        {roleId(Role::Favicon), QByteArrayLiteral("favicon")},
    };
}

int BookmarkModel::count() const
{
    return m_bookmarks.count();
}

QString BookmarkModel::activeUrl() const
{
    return m_activeUrl;
}

void BookmarkModel::setActiveUrl(const QString &url)
{
    if (m_activeUrl == url) {
        return;
    }
    m_activeUrl = url;
    emit activeUrlChanged();
    emit activeUrlBookmarkedChanged();
}

bool BookmarkModel::activeUrlBookmarked() const
{
    return contains(m_activeUrl);
}

int BookmarkModel::revision() const
{
    return m_revision;
}

const QList<BookmarkModel::Bookmark> &BookmarkModel::bookmarks() const
{
    return m_bookmarks;
}

bool BookmarkModel::hasBookmark(int id) const
{
    return indexOfId(m_bookmarks, id) >= 0;
}

QString BookmarkModel::urlOf(int id) const
{
    const int index = indexOfId(m_bookmarks, id);
    return index >= 0 ? m_bookmarks.at(index).url : QString();
}

QString BookmarkModel::titleOf(int id) const
{
    const int index = indexOfId(m_bookmarks, id);
    return index >= 0 ? shownTitle(m_bookmarks.at(index)) : QString();
}

int BookmarkModel::idForUrl(const QString &url) const
{
    const int index = indexOfUrl(m_bookmarks, url);
    return index >= 0 ? m_bookmarks.at(index).id : 0;
}

QVariantList BookmarkModel::matching(const QString &query) const
{
    const SearchWords words(query);
    QVariantList found;
    for (const Bookmark &bookmark : m_bookmarks) {
        if (!words.matches({bookmark.title, bookmark.url})) {
            continue;
        }
        found.append(QVariantMap{
            {QStringLiteral("bookmarkId"), bookmark.id},
            {QStringLiteral("title"), shownTitle(bookmark)},
            {QStringLiteral("url"), bookmark.url},
            {QStringLiteral("favicon"), bookmark.favicon},
        });
    }
    return found;
}

int BookmarkModel::add(const QString &url, const QString &title, const QString &favicon)
{
    if (url.isEmpty()) {
        return 0;
    }
    const int existing = indexOfUrl(m_bookmarks, url);
    if (existing >= 0) {
        return m_bookmarks.at(existing).id;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT INTO bookmark (url, title, favicon, position, created) "
                                 "VALUES (?, ?, ?, "
                                 "(SELECT COALESCE(MAX(position), 0) + 1 FROM bookmark), ?)"));
    query.addBindValue(url);
    query.addBindValue(Storage::text(title));
    query.addBindValue(Storage::text(favicon));
    const qint64 created = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;
    query.addBindValue(created);
    if (!run(query)) {
        return 0;
    }

    Bookmark bookmark;
    bookmark.id = query.lastInsertId().toInt();
    bookmark.url = url;
    bookmark.title = title;
    bookmark.favicon = favicon;
    bookmark.created = created * 1000;

    const int index = m_bookmarks.count();
    beginInsertRows(QModelIndex(), index, index);
    m_bookmarks.append(bookmark);
    endInsertRows();
    emit countChanged();
    emit activeUrlBookmarkedChanged();
    bump();
    return bookmark.id;
}

void BookmarkModel::remove(int index)
{
    if (index < 0 || index >= m_bookmarks.count()) {
        return;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM bookmark WHERE id = ?"));
    query.addBindValue(m_bookmarks.at(index).id);
    if (!run(query)) {
        return;
    }
    beginRemoveRows(QModelIndex(), index, index);
    m_bookmarks.removeAt(index);
    endRemoveRows();
    emit countChanged();
    emit activeUrlBookmarkedChanged();
    bump();
}

bool BookmarkModel::removeByUrl(const QString &url)
{
    const int index = indexOfUrl(m_bookmarks, url);
    if (index < 0) {
        return false;
    }
    remove(index);
    return true;
}

void BookmarkModel::edit(int index, const QString &url, const QString &title)
{
    if (index < 0 || index >= m_bookmarks.count() || url.isEmpty()) {
        return;
    }
    Bookmark &bookmark = m_bookmarks[index];
    if (bookmark.url == url && bookmark.title == title) {
        return;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE bookmark SET url = ?, title = ? WHERE id = ?"));
    query.addBindValue(url);
    query.addBindValue(Storage::text(title));
    query.addBindValue(bookmark.id);
    if (!run(query)) {
        return;
    }
    QVector<int> roles;
    if (bookmark.url != url) {
        bookmark.url = url;
        roles.append(roleId(Role::Url));
    }
    if (bookmark.title != title) {
        bookmark.title = title;
        roles.append(roleId(Role::Title));
    }
    notifyRow(index, roles);
    emit activeUrlBookmarkedChanged();
    bump();
}

bool BookmarkModel::contains(const QString &url) const
{
    return indexOfUrl(m_bookmarks, url) >= 0;
}

void BookmarkModel::updateFavicon(const QString &url, const QString &favicon)
{
    const int index = indexOfUrl(m_bookmarks, url);
    if (index < 0 || m_bookmarks.at(index).favicon == favicon) {
        return;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE bookmark SET favicon = ? WHERE id = ?"));
    query.addBindValue(favicon);
    query.addBindValue(m_bookmarks.at(index).id);
    if (!run(query)) {
        return;
    }
    m_bookmarks[index].favicon = favicon;
    notifyRow(index, QVector<int>{roleId(Role::Favicon)});
    bump();
}

void BookmarkModel::clear()
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM bookmark"));
    if (run(query)) {
        reload();
    }
}

void BookmarkModel::notifyRow(int index, const QVector<int> &roles)
{
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, roles);
}

void BookmarkModel::reload()
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT id, url, title, favicon, created FROM bookmark "
                                 "ORDER BY position ASC"));
    if (!run(query)) {
        return;
    }
    QList<Bookmark> bookmarks;
    while (query.next()) {
        Bookmark bookmark;
        bookmark.id = query.value(0).toInt();
        bookmark.url = query.value(1).toString();
        bookmark.title = query.value(2).toString();
        bookmark.favicon = query.value(3).toString();
        bookmark.created = query.value(4).toLongLong() * 1000;
        bookmarks.append(bookmark);
    }
    const int oldCount = m_bookmarks.count();
    beginResetModel();
    m_bookmarks = bookmarks;
    endResetModel();
    if (oldCount != m_bookmarks.count()) {
        emit countChanged();
    }
    emit activeUrlBookmarkedChanged();
    bump();
}

void BookmarkModel::bump()
{
    ++m_revision;
    emit revisionChanged();
}

} // namespace Salama
