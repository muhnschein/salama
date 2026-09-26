// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SettingsSections.h"

namespace Salama {

SettingsSections::SettingsSections(const QString &filePath)
    : m_file(filePath, QSettings::IniFormat)
    , m_general(m_file)
    , m_search(m_file)
    , m_reader(m_file)
    , m_cover(m_file)
    , m_privacy(m_file)
    , m_startPage(m_file)
{
}

Settings *SettingsSections::general()
{
    return &m_general;
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

StartPageSettings *SettingsSections::startPage()
{
    return &m_startPage;
}

} // namespace Salama
