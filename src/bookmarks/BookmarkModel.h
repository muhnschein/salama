// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Modelled on sailfish-browser apps/browser/bookmarks/declarativebookmarkmodel.{h,cpp}
// (Copyright (c) 2013 - 2021 Jolla Ltd., MPL-2.0), stored in SQLite instead of JSON so
// Phase 2 folders are a column, not a file-format change.
#pragma once

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
    // Counts every change to the bookmarks: one added, removed or edited, a favicon
    // arriving, the list cleared or read again. A QML binding that asks one of the
    // invokables below reads this first -- `(BookmarkModel.revision,
    // BookmarkModel.titleOf(id))` -- so that it is asked again when anything changes,
    // which a call alone would never be.
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    enum Role
    {
        BookmarkIdRole = Qt::UserRole + 1,
        UrlRole,
        TitleRole,
        FaviconRole
    };

    struct Bookmark
    {
        int id = 0;
        QString url;
        QString title;
        QString favicon;
    };

    explicit BookmarkModel(Storage &storage, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString activeUrl() const;
    void setActiveUrl(const QString &url);
    bool activeUrlBookmarked() const;
    int revision() const;
    // The bookmarks in the list's order, for the address bar's suggestions
    // (docs/DECISIONS/0027-omnibar.md): each title as stored, empty when there is none.
    const QList<Bookmark> &bookmarks() const;

    // Returns the bookmark id; an already bookmarked url returns its existing id.
    Q_INVOKABLE int add(const QString &url, const QString &title,
                        const QString &favicon = QString());
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE bool removeByUrl(const QString &url);
    Q_INVOKABLE void edit(int index, const QString &url, const QString &title);
    Q_INVOKABLE bool contains(const QString &url) const;
    Q_INVOKABLE void updateFavicon(const QString &url, const QString &favicon);
    Q_INVOKABLE void clear();

    // One bookmark by its id, for the cover's quick action, which keeps a bookmark by
    // its id (docs/DECISIONS/0029-quick-action.md). The title is the one the list
    // shows, the address when there is no other; an id with no bookmark has neither.
    Q_INVOKABLE bool hasBookmark(int id) const;
    Q_INVOKABLE QString urlOf(int id) const;
    Q_INVOKABLE QString titleOf(int id) const;
    // The id of the bookmark for this address, 0 when there is none: a bookmark
    // removed and added again comes back under another id, and is found by this.
    Q_INVOKABLE int idForUrl(const QString &url) const;
    // The bookmarks whose title and address hold every word of the query, in the
    // list's order, as maps of bookmarkId, title, url and favicon -- the title as
    // titleOf() gives it. Every bookmark for an empty query. What the quick action's
    // bookmark picker lists; matched as every search in the browser is (SearchWords).
    Q_INVOKABLE QVariantList matching(const QString &query) const;

signals:
    void countChanged();
    void activeUrlChanged();
    void activeUrlBookmarkedChanged();
    void revisionChanged();

private:
    static QString shownTitle(const Bookmark &bookmark);
    int indexOfId(int id) const;
    int indexOfUrl(const QString &url) const;
    void notifyRow(int index, const QVector<int> &roles);
    void reload();
    void bump();

    QSqlDatabase m_db;
    QList<Bookmark> m_bookmarks;
    QString m_activeUrl;
    int m_revision = 0;
};

} // namespace Salama
