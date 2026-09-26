// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SettingsSections.h"

namespace Salama {

SettingsSections::SettingsSections(const QString &filePath)
    : file(filePath, QSettings::IniFormat)
    , general(file)
    , search(file)
    , reader(file)
    , cover(file)
    , privacy(file)
    , startPage(file)
{
}

} // namespace Salama
