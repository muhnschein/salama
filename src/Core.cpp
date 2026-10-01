// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Core.h"

#include "tabs/ClosedTabModel.h"

#include <initializer_list>

namespace Salama {

Core::Core(const QString &dataDirectory, const QString &configFilePath,
           const QString &downloadDirectory, QObject *parent)
    : QObject(parent)
    , m_storage(dataDirectory)
    , m_tabPersistence(m_storage)
    , m_tabs(&m_tabPersistence, Storage::defaultCacheDirectory())
    , m_tabSearch(&m_tabs)
    , m_history(m_storage)
    , m_bookmarks(m_storage)
    , m_downloads(m_storage, downloadDirectory)
    , m_settings(configFilePath)
    , m_omnibar(&m_tabs, &m_bookmarks, &m_history, &m_downloads, m_settings.search(),
                m_settings.privacy())
    , m_pageMedia(&m_tabs)
    , m_reader(*m_settings.reader())
    , m_startPage(m_storage, *m_settings.searchEngines())
    , m_webNotifications(&m_notificationPermissions,
                         Storage::defaultCacheDirectory() + QStringLiteral("/notifications"))
{
    // Unless the history is not to be kept.
    connect(&m_tabs, &TabModel::visited, &m_history, [this](const QString &url) {
        if (m_settings.privacy()->rememberHistory()) {
            m_history.visit(url);
        }
    });
    connect(&m_tabs, &TabModel::titleUpdated, &m_history, &HistoryModel::updateTitle);
    connect(&m_tabs, &TabModel::faviconUpdated, &m_bookmarks, &BookmarkModel::updateFavicon);
    connect(&m_tabs, &TabModel::faviconUpdated, &m_history, &HistoryModel::updateFavicon);
    // The start page is read from the history and the bookmarks, and again when either
    // changes, icons included (docs/DECISIONS/0032-start-page.md).
    const std::initializer_list<const QAbstractItemModel *> startPageSources{&m_history,
                                                                             &m_bookmarks};
    for (const QAbstractItemModel *source : startPageSources) {
        connect(source, &QAbstractItemModel::modelReset, &m_startPage, &StartPage::refresh);
        connect(source, &QAbstractItemModel::rowsInserted, &m_startPage, &StartPage::refresh);
        connect(source, &QAbstractItemModel::rowsRemoved, &m_startPage, &StartPage::refresh);
        connect(source, &QAbstractItemModel::dataChanged, &m_startPage, &StartPage::refresh);
    }
    // An engine added or removed changes which pages are searches and which are sites
    // (docs/DECISIONS/0041-search-engines-found.md).
    connect(m_settings.searchEngines(), &SearchEngines::enginesChanged, &m_startPage,
            &StartPage::refresh);
    connect(&m_tabs, &TabModel::activeTabDataChanged, &m_bookmarks,
            [this]() { m_bookmarks.setActiveUrl(m_tabs.activeUrl()); });
    m_bookmarks.setActiveUrl(m_tabs.activeUrl());

    // The notifications' permission is kept by two models, the notifications' own and
    // the site permissions' (docs/DECISIONS/0039-site-permissions.md); each takes in what
    // the other decided, which the engine has been told of already.
    connect(&m_sitePermissions, &SitePermissions::decided, &m_notificationPermissions,
            [this](int kind, const QString &origin, int decision) {
                if (kind == SitePermissions::Notifications) {
                    m_notificationPermissions.adopt(origin, decision);
                }
            });
    connect(&m_notificationPermissions, &NotificationPermissions::decided, &m_sitePermissions,
            [this](const QString &origin, int capability) {
                m_sitePermissions.adopt(SitePermissions::Notifications, origin, capability);
            });

    // Five pages stay loaded, as in Jolla's browser.
    m_tabs.setLiveTabLimit(TabModel::LiveTabLimit);

    // The engine says something started or stopped playing, and not where; the pages
    // are asked (docs/DECISIONS/0026-media-controls.md).
    connect(&m_pageActivity, &PageActivity::playStateChanged, &m_pageMedia, &PageMedia::refresh);
    connect(&m_pageActivity, &PageActivity::backgroundChanged, &m_pageMedia,
            [this]() { m_pageMedia.setBackground(m_pageActivity.background()); });

    clearOnClose();
}

void Core::clearOnClose()
{
    if (!m_settings.privacy()->clearHistoryOnClose()) {
        return;
    }
    m_history.clear();
    m_downloads.clear();
    m_tabs.closedTabs()->clear();
}

Storage &Core::storage()
{
    return m_storage;
}

TabModel *Core::tabs()
{
    return &m_tabs;
}

TabSearchModel *Core::tabSearch()
{
    return &m_tabSearch;
}

HistoryModel *Core::history()
{
    return &m_history;
}

BookmarkModel *Core::bookmarks()
{
    return &m_bookmarks;
}

DownloadModel *Core::downloads()
{
    return &m_downloads;
}

Settings *Core::settings()
{
    return m_settings.general();
}

SearchEngines *Core::searchEngines()
{
    return m_settings.searchEngines();
}

SearchSettings *Core::searchSettings()
{
    return m_settings.search();
}

ReaderSettings *Core::readerSettings()
{
    return m_settings.reader();
}

CoverSettings *Core::coverSettings()
{
    return m_settings.cover();
}

PrivacySettings *Core::privacySettings()
{
    return m_settings.privacy();
}

StartPageSettings *Core::startPageSettings()
{
    return m_settings.startPage();
}

SitePermissionSettings *Core::sitePermissionSettings()
{
    return m_settings.sitePermissions();
}

OmnibarModel *Core::omnibar()
{
    return &m_omnibar;
}

EngineMessages *Core::engineMessages()
{
    return &m_engineMessages;
}

PageActivity *Core::pageActivity()
{
    return &m_pageActivity;
}

PageMedia *Core::pageMedia()
{
    return &m_pageMedia;
}

Reader *Core::reader()
{
    return &m_reader;
}

StartPage *Core::startPage()
{
    return &m_startPage;
}

ShareReceiver *Core::shareReceiver()
{
    return &m_shareReceiver;
}

NotificationPermissions *Core::notificationPermissions()
{
    return &m_notificationPermissions;
}

SitePermissions *Core::sitePermissions()
{
    return &m_sitePermissions;
}

WebNotifications *Core::webNotifications()
{
    return &m_webNotifications;
}

} // namespace Salama
