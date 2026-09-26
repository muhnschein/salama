// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SettingsSections.h"

namespace Salama {

SettingsSections::SettingsSections(const QString &filePath)
    : m_file(filePath, QSettings::IniFormat)
    , general(m_file)
    , search(m_file)
    , reader(m_file)
    , cover(m_file)
    , privacy(m_file)
    , startPage(m_file)
{
}

} // namespace Salama
