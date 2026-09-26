// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

// Settings > Start page: what the start page shows, nothing at all or the sections
// switched on (docs/DECISIONS/0032-start-page.md). The sections are each on until they
// are switched off, and keep their switches while the page is blank, for when it is
// not.
class StartPageSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(bool blank READ blank WRITE setBlank NOTIFY changed)
    Q_PROPERTY(bool topSites READ topSites WRITE setTopSites NOTIFY changed)
    Q_PROPERTY(bool bookmarks READ bookmarks WRITE setBookmarks NOTIFY changed)
    Q_PROPERTY(bool recent READ recent WRITE setRecent NOTIFY changed)

public:
    // Forgets the home page an earlier release kept, whose place the start page took.
    explicit StartPageSettings(QSettings &file, QObject *parent = nullptr);

    bool blank() const;
    void setBlank(bool blank);
    bool topSites() const;
    void setTopSites(bool shown);
    bool bookmarks() const;
    void setBookmarks(bool shown);
    bool recent() const;
    void setRecent(bool shown);

signals:
    void changed();
};

} // namespace Salama
