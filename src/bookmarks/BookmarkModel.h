// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/browser/bookmarks/declarativebookmarkmodel.{h,cpp}
// (Copyright (c) 2013 - 2021 Jolla Ltd., MPL-2.0). SQLite not JSON: folders = column later.
#pragma once

#include "ModelRoles.h"

#include <QAbstractListModel>
#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QVariantList>

namespace Salama {

class Storage;

class BookmarkModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString activeUrl READ activeUrl WRITE setActiveUrl NOTIFY activeUrlChanged)
    Q_PROPERTY(bool activeUrlBookmarked READ activeUrlBookmarked NOTIFY activeUrlBookmarkedChanged)
    // Bumps on any change. QML bindings calling invokables read it first so they re-evaluate:
    // `(BookmarkModel.revision, BookmarkModel.titleOf(id))`.
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    enum class Role
    {
        BookmarkId = Qt::UserRole + 1,
        Url,
        Title,
        Favicon
    };

    struct Bookmark
    {
        int id = 0;
        QString url;
        QString title;
        QString favicon;
        // Added time, ms since epoch. Age for never-visited bookmark in OmnibarModel::frecency.
        qint64 created = 0;
    };

    explicit BookmarkModel(const Storage &storage, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString activeUrl() const;
    void setActiveUrl(const QString &url);
    bool activeUrlBookmarked() const;
    int revision() const;
    const QList<Bookmark> &bookmarks() const;

    Q_INVOKABLE int add(const QString &url, const QString &title,
                        const QString &favicon = QString());
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE bool removeByUrl(const QString &url);
    Q_INVOKABLE void edit(int index, const QString &url, const QString &title);
    Q_INVOKABLE bool contains(const QString &url) const;
    Q_INVOKABLE void updateFavicon(const QString &url, const QString &favicon);
    Q_INVOKABLE void clear();

    // Title falls back to url.
    Q_INVOKABLE bool hasBookmark(int id) const;
    Q_INVOKABLE QString urlOf(int id) const;
    Q_INVOKABLE QString titleOf(int id) const;
    // 0 if none.
    Q_INVOKABLE int idForUrl(const QString &url) const;
    // Maps of bookmarkId, title, url, favicon. Empty query -> all.
    Q_INVOKABLE QVariantList matching(const QString &query) const;

signals:
    void countChanged();
    void activeUrlChanged();
    void activeUrlBookmarkedChanged();
    void revisionChanged();

private:
    void notifyRow(int index, const QVector<int> &roles);
    void reload();
    void bump();

    QSqlDatabase m_db;
    QList<Bookmark> m_bookmarks;
    QString m_activeUrl;
    int m_revision = 0;
};

} // namespace Salama
