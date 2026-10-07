// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "bookmarks/BookmarkModel.h"
#include "downloads/DownloadModel.h"
#include "engine/EngineMessages.h"
#include "engine/PageActivity.h"
#include "engine/PageMedia.h"
#include "history/HistoryModel.h"
#include "notifications/NotificationPermissions.h"
#include "notifications/WebNotifications.h"
#include "omnibar/OmnibarModel.h"
#include "permissions/SitePermissions.h"
#include "reader/Reader.h"
#include "settings/SettingsSections.h"
#include "share/ShareReceiver.h"
#include "startpage/StartPage.h"
#include "storage/Storage.h"
#include "tabs/TabModel.h"
#include "tabs/TabPersistence.h"
#include "tabs/TabSearchModel.h"

#include <QObject>
#include <QString>

namespace Salama {

// Owns all models + wiring. One per process; tests build one per case on temp dir.
class Core : public QObject
{
    Q_OBJECT

public:
    Core(const QString &dataDirectory, const QString &configFilePath,
         const QString &downloadDirectory, QObject *parent = nullptr);

    Storage &storage();
    TabModel *tabs();
    TabSearchModel *tabSearch();
    HistoryModel *history();
    BookmarkModel *bookmarks();
    DownloadModel *downloads();
    Settings *settings();
    SearchEngines *searchEngines();
    SearchSettings *searchSettings();
    ReaderSettings *readerSettings();
    CoverSettings *coverSettings();
    PrivacySettings *privacySettings();
    DohSettings *dohSettings();
    StartPageSettings *startPageSettings();
    SitePermissionSettings *sitePermissionSettings();
    OmnibarModel *omnibar();
    EngineMessages *engineMessages();
    PageActivity *pageActivity();
    PageMedia *pageMedia();
    Reader *reader();
    StartPage *startPage();
    NotificationPermissions *notificationPermissions();
    SitePermissions *sitePermissions();
    WebNotifications *webNotifications();
    ShareReceiver *shareReceiver();

    // Called on quit and on start too: browser may be killed before quit.
    void clearOnClose();

private:
    Storage m_storage;
    TabPersistence m_tabPersistence;
    TabModel m_tabs;
    TabSearchModel m_tabSearch;
    HistoryModel m_history;
    BookmarkModel m_bookmarks;
    DownloadModel m_downloads;
    SettingsSections m_settings;
    // Declared after everything it searches: init order.
    OmnibarModel m_omnibar;
    EngineMessages m_engineMessages;
    PageActivity m_pageActivity;
    PageMedia m_pageMedia;
    Reader m_reader;
    StartPage m_startPage;
    NotificationPermissions m_notificationPermissions;
    SitePermissions m_sitePermissions;
    // After permissions: reads them at init.
    WebNotifications m_webNotifications;
    ShareReceiver m_shareReceiver;
};

} // namespace Salama
