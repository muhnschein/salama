// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "CoverSettings.h"
#include "PrivacySettings.h"
#include "ReaderSettings.h"
#include "SearchSettings.h"
#include "Settings.h"
#include "StartPageSettings.h"

#include <QSettings>
#include <QString>

namespace Salama {

// The settings file and a section per settings page over it: what Core keeps, and what
// the tests build over a file of their own.
class SettingsSections
{
public:
    explicit SettingsSections(const QString &filePath);

    Settings *general();
    SearchSettings *search();
    ReaderSettings *reader();
    CoverSettings *cover();
    PrivacySettings *privacy();
    StartPageSettings *startPage();

private:
    // First, so that it is made before the sections that borrow it and goes after them.
    QSettings m_file;
    Settings m_general;
    SearchSettings m_search;
    ReaderSettings m_reader;
    CoverSettings m_cover;
    PrivacySettings m_privacy;
    StartPageSettings m_startPage;
};

} // namespace Salama
