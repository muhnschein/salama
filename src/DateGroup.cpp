// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "DateGroup.h"

#include <QDateTime>

namespace Salama {

DateGroup::Group DateGroup::of(const QDate &day, const QDate &today)
{
    const qint64 daysAgo = day.daysTo(today);
    if (daysAgo <= 0) {
        return Today;
    }
    if (daysAgo == 1) {
        return Yesterday;
    }
    // Today counts as one of the 7 and 30.
    if (daysAgo < 7) {
        return LastSevenDays;
    }
    if (daysAgo < 30) {
        return LastThirtyDays;
    }
    return Older;
}

DateGroup::Group DateGroup::of(qint64 time)
{
    return of(QDateTime::fromMSecsSinceEpoch(time).date(), QDate::currentDate());
}

} // namespace Salama
