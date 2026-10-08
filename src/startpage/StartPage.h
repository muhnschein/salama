// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SiteListModel.h"

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace Salama {

class SearchEngines;
class Storage;

// Search result pages excluded, else engine tops list.
class StartPage : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Salama::SiteListModel *topSites READ topSites CONSTANT)
    Q_PROPERTY(Salama::SiteListModel *bookmarks READ bookmarks CONSTANT)
    Q_PROPERTY(Salama::SiteListModel *recentPages READ recentPages CONSTANT)

public:
    static const int TopSiteLimit = 8;
    static const int BookmarkLimit = 8;
    static const int RecentPageLimit = 5;

    // `engines` must outlive this.
    StartPage(const Storage &storage, const SearchEngines &engines, QObject *parent = nullptr);

    SiteListModel *topSites();
    SiteListModel *bookmarks();
    SiteListModel *recentPages();

    Q_INVOKABLE void refresh();

    static QString siteOf(const QString &url);

private:
    QList<Site> readHistory(const QString &order, int limit, bool onePerSite) const;
    QList<Site> readBookmarks() const;
    static Site siteAt(const QSqlQuery &query);

    QSqlDatabase m_db;
    const SearchEngines &m_engines;
    SiteListModel m_topSites;
    SiteListModel m_bookmarks;
    SiteListModel m_recentPages;
};

} // namespace Salama
