// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "CoverSettings.h"
#include "DohSettings.h"
#include "PrivacySettings.h"
#include "ReaderSettings.h"
#include "SearchSettings.h"
#include "Settings.h"
#include "SitePermissionSettings.h"
#include "StartPageSettings.h"
#include "search/SearchEngines.h"

#include <QSettings>
#include <QString>

namespace Salama {

class SettingsSections
{
public:
    explicit SettingsSections(const QString &filePath);

    Settings *general();
    SearchEngines *searchEngines();
    SearchSettings *search();
    ReaderSettings *reader();
    CoverSettings *cover();
    PrivacySettings *privacy();
    DohSettings *doh();
    StartPageSettings *startPage();
    SitePermissionSettings *sitePermissions();

private:
    // First: sections borrow it.
    QSettings m_file;
    Settings m_general;
    // Before m_search, which borrows it.
    SearchEngines m_searchEngines;
    SearchSettings m_search;
    ReaderSettings m_reader;
    CoverSettings m_cover;
    PrivacySettings m_privacy;
    DohSettings m_doh;
    StartPageSettings m_startPage;
    SitePermissionSettings m_sitePermissions;
};

} // namespace Salama
