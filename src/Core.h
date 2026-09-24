// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "bookmarks/BookmarkModel.h"
#include "downloads/DownloadModel.h"
#include "engine/EngineMessages.h"
#include "engine/PageActivity.h"
#include "history/HistoryModel.h"
#include "reader/Reader.h"
#include "settings/Settings.h"
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
    Core(const QString &dataDirectory, const QString &configFilePath, QObject *parent = nullptr);

    Storage &storage();
    TabModel *tabs();
    TabSearchModel *tabSearch();
    HistoryModel *history();
    BookmarkModel *bookmarks();
    DownloadModel *downloads();
    Settings *settings();
    EngineMessages *engineMessages();
    PageActivity *pageActivity();
    Reader *reader();

private:
    Storage m_storage;
    TabPersistence m_tabPersistence;
    TabModel m_tabs;
    TabSearchModel m_tabSearch;
    HistoryModel m_history;
    BookmarkModel m_bookmarks;
    DownloadModel m_downloads;
    Settings m_settings;
    EngineMessages m_engineMessages;
    PageActivity m_pageActivity;
    Reader m_reader;
};

} // namespace Salama
