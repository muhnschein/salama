// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "EngineData.h"

#include <cmath>
#include <limits>

namespace Salama {

namespace EngineData {

bool isNumber(const QVariant &value)
{
    const int type = value.userType();
    return type == QMetaType::Double || type == QMetaType::Int || type == QMetaType::UInt ||
           type == QMetaType::LongLong || type == QMetaType::ULongLong;
}

int id(const QVariant &value)
{
    if (!isNumber(value)) {
        return 0;
    }
    const double number = value.toDouble();
    // A NaN fails the last of these: it equals nothing, itself included.
    if (number < 1 || number > std::numeric_limits<int>::max() || std::floor(number) != number) {
        return 0;
    }
    return static_cast<int>(number);
}

} // namespace EngineData

} // namespace Salama
