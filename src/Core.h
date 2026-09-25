// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "bookmarks/BookmarkModel.h"
#include "downloads/DownloadModel.h"
#include "engine/EngineMessages.h"
#include "engine/PageActivity.h"
#include "engine/PageMedia.h"
#include "history/HistoryModel.h"
#include "omnibar/OmnibarModel.h"
#include "reader/Reader.h"
#include "settings/Settings.h"
#include "startpage/StartPage.h"
#include "storage/Storage.h"
#include "tabs/TabModel.h"
#include "tabs/TabPersistence.h"
#include "tabs/TabSearchModel.h"

#include <QObject>
#include <QString>

namespace Salama {

// Owns every model and the wiring between them. One per process; tests build one
// per test case on a temporary directory.
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
    OmnibarModel *omnibar();
    EngineMessages *engineMessages();
    PageActivity *pageActivity();
    PageMedia *pageMedia();
    Reader *reader();
    StartPage *startPage();

    // What is set to go as the browser closes -- the history, the list of downloads and
    // the recently closed tabs, with Settings::clearHistoryOnClose -- goes: main() calls
    // it as the application quits, and the constructor on every start, for a browser
    // stopped before it could (docs/DECISIONS/0030-history-settings.md).
    void clearOnClose();

private:
    Storage m_storage;
    TabPersistence m_tabPersistence;
    TabModel m_tabs;
    TabSearchModel m_tabSearch;
    HistoryModel m_history;
    BookmarkModel m_bookmarks;
    DownloadModel m_downloads;
    Settings m_settings;
    // After everything it searches, which it is made from.
    OmnibarModel m_omnibar;
    EngineMessages m_engineMessages;
    PageActivity m_pageActivity;
    PageMedia m_pageMedia;
    Reader m_reader;
    StartPage m_startPage;
};

} // namespace Salama
