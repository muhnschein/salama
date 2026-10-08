// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QVariant>

namespace Salama {

// Engine numbers arrive as double (Qt 5.6), qlonglong (Qt 5.15+) or int (QML). Check by type:
// QVariant would convert strings/false and round fractions.
namespace EngineData {

bool isNumber(const QVariant &value);

// 1 up; 0 if not whole number in int range.
int id(const QVariant &value);

} // namespace EngineData

} // namespace Salama
