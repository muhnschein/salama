// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SettingsSections.h"

namespace Salama {

SettingsSections::SettingsSections(const QString &filePath)
    : m_file(filePath, QSettings::IniFormat)
    , m_general(m_file)
    , m_searchEngines(m_file)
    , m_search(m_file, m_searchEngines)
    , m_reader(m_file)
    , m_cover(m_file)
    , m_privacy(m_file)
    , m_doh(m_file)
    , m_startPage(m_file)
    , m_sitePermissions(m_file)
{
}

Settings *SettingsSections::general()
{
    return &m_general;
}

SearchEngines *SettingsSections::searchEngines()
{
    return &m_searchEngines;
}

SearchSettings *SettingsSections::search()
{
    return &m_search;
}

ReaderSettings *SettingsSections::reader()
{
    return &m_reader;
}

CoverSettings *SettingsSections::cover()
{
    return &m_cover;
}

PrivacySettings *SettingsSections::privacy()
{
    return &m_privacy;
}

DohSettings *SettingsSections::doh()
{
    return &m_doh;
}

StartPageSettings *SettingsSections::startPage()
{
    return &m_startPage;
}

SitePermissionSettings *SettingsSections::sitePermissions()
{
    return &m_sitePermissions;
}

} // namespace Salama
