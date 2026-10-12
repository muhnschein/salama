// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

class StartPageSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(bool blank READ blank WRITE setBlank NOTIFY changed)
    Q_PROPERTY(bool topSites READ topSites WRITE setTopSites NOTIFY changed)
    Q_PROPERTY(int topSiteRows READ topSiteRows WRITE setTopSiteRows NOTIFY changed)
    Q_PROPERTY(bool bookmarks READ bookmarks WRITE setBookmarks NOTIFY changed)
    Q_PROPERTY(bool recent READ recent WRITE setRecent NOTIFY changed)

public:
    static const int MinTopSiteRows = 1;
    static const int MaxTopSiteRows = 2;

    explicit StartPageSettings(QSettings &file, QObject *parent = nullptr);

    bool blank() const;
    void setBlank(bool blank);
    bool topSites() const;
    void setTopSites(bool shown);
    // Rows of frequently visited tiles; outside [MinTopSiteRows, MaxTopSiteRows] refused.
    int topSiteRows() const;
    void setTopSiteRows(int rows);
    bool bookmarks() const;
    void setBookmarks(bool shown);
    bool recent() const;
    void setRecent(bool shown);

signals:
    void changed();
};

} // namespace Salama
