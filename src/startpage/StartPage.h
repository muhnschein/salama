// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SiteListModel.h"

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace Salama {

class Storage;

// What a tab with no address shows, in place of a home page: the lists Firefox's home
// offers, read from the history and the bookmarks (docs/DECISIONS/0032-start-page.md).
// The sites visited most, one tile per site; the first bookmarks, in their order; and
// the pages read last. A page of search results is in neither history list: a search is
// something done, not a site visited, and every search would otherwise make its engine
// the site visited most. Which lists the page shows is the settings' to say; all three
// are kept current whatever they say, which costs a few small queries per change.
class StartPage : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Salama::SiteListModel *topSites READ topSites CONSTANT)
    Q_PROPERTY(Salama::SiteListModel *bookmarks READ bookmarks CONSTANT)
    Q_PROPERTY(Salama::SiteListModel *recentPages READ recentPages CONSTANT)

public:
    // Two rows of four tiles each, and a handful of rows.
    static const int TopSiteLimit = 8;
    static const int BookmarkLimit = 8;
    static const int RecentPageLimit = 5;

    explicit StartPage(const Storage &storage, QObject *parent = nullptr);

    SiteListModel *topSites();
    SiteListModel *bookmarks();
    SiteListModel *recentPages();

    // Reads the three lists again. Core has it done whenever the history or the
    // bookmarks change.
    Q_INVOKABLE void refresh();

    // What makes two pages one site: the host without "www.", or the whole address
    // of one that has no host.
    static QString siteOf(const QString &url);

private:
    // The history's pages in the order given, searches left out, until there are
    // `limit` of them; with `onePerSite`, only the first page of each site.
    QList<Site> readHistory(const QString &order, int limit, bool onePerSite) const;
    QList<Site> readBookmarks() const;
    static Site siteAt(const QSqlQuery &query);

    QSqlDatabase m_db;
    SiteListModel m_topSites;
    SiteListModel m_bookmarks;
    SiteListModel m_recentPages;
};

} // namespace Salama
