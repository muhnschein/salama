// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QDate>
#include <QObject>

namespace Salama {

// History's and Downloads' sections, Firefox for Android's: Today, Yesterday, Last 7 days,
// Last 30 days, Older; running and paused downloads above them, In progress. Calendar days:
// yesterday starts at last midnight. QObject only for QML's enum.
class DateGroup : public QObject
{
    Q_OBJECT

public:
    enum Group // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        InProgress,
        Today,
        Yesterday,
        LastSevenDays,
        LastThirtyDays,
        Older
    };
    Q_ENUM(Group)

    // Later than today (clock moved back): Today.
    static Group of(const QDate &day, const QDate &today);
    // ms since epoch, against local today.
    static Group of(qint64 time);
};

} // namespace Salama
