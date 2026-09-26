// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "StartPage.h"

#include "settings/SearchSettings.h"
#include "storage/Storage.h"

#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

namespace Salama {

namespace {

bool run(QSqlQuery &query)
{
    if (!query.exec()) {
        qWarning() << "StartPage:" << query.lastError().text() << query.lastQuery();
        return false;
    }
    return true;
}

} // namespace

StartPage::StartPage(const Storage &storage, QObject *parent)
    : QObject(parent)
    , m_db(storage.database())
{
    refresh();
}

SiteListModel *StartPage::topSites()
{
    return &m_topSites;
}

SiteListModel *StartPage::bookmarks()
{
    return &m_bookmarks;
}

SiteListModel *StartPage::recentPages()
{
    return &m_recentPages;
}

void StartPage::refresh()
{
    // Visited most, and of two pages visited as often the one visited last: a tile
    // stands for the page of its site that was read most.
    m_topSites.setSites(
        readHistory(QStringLiteral("visited_count DESC, date DESC, id DESC"), TopSiteLimit, true));
    m_bookmarks.setSites(readBookmarks());
    m_recentPages.setSites(
        readHistory(QStringLiteral("date DESC, id DESC"), RecentPageLimit, false));
}

QString StartPage::siteOf(const QString &url)
{
    return SearchSettings::displayAddress(url);
}

Site StartPage::siteAt(const QSqlQuery &query)
{
    Site site;
    site.url = query.value(0).toString();
    site.title = query.value(1).toString();
    site.favicon = query.value(2).toString();
    return site;
}

// The whole history is read in the worst case -- a history of one site, or of
// searches -- which its cap of HistoryModel::MaxEntries rows keeps small.
QList<Site> StartPage::readHistory(const QString &order, int limit, bool onePerSite) const
{
    QSqlQuery query(m_db);
    query.setForwardOnly(true);
    query.prepare(
        QStringLiteral("SELECT url, title, favicon FROM browser_history ORDER BY %1").arg(order));
    if (!run(query)) {
        return {};
    }
    QList<Site> sites;
    QSet<QString> seen;
    while (sites.count() < limit && query.next()) {
        const Site site = siteAt(query);
        if (SearchSettings::isSearchUrl(site.url)) {
            continue;
        }
        if (onePerSite) {
            const QString key = siteOf(site.url);
            if (seen.contains(key)) {
                continue;
            }
            seen.insert(key);
        }
        sites.append(site);
    }
    return sites;
}

QList<Site> StartPage::readBookmarks() const
{
    QSqlQuery query(m_db);
    query.setForwardOnly(true);
    query.prepare(QStringLiteral(
        "SELECT url, title, favicon FROM bookmark ORDER BY position ASC, id ASC LIMIT ?"));
    query.addBindValue(BookmarkLimit);
    if (!run(query)) {
        return {};
    }
    QList<Site> sites;
    while (query.next()) {
        sites.append(siteAt(query));
    }
    return sites;
}

} // namespace Salama
