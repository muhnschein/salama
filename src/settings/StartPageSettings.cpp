// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "StartPageSettings.h"

namespace Salama {

namespace {

const char *const BlankKey = "startPageBlank";
const char *const TopSitesKey = "startPageTopSites";
const char *const BookmarksKey = "startPageBookmarks";
const char *const RecentKey = "startPageRecent";
// Where an earlier release kept the address of its home page. The start page took the
// home page's place, and nothing reads it now.
const char *const RetiredHomePageKey = "homePage";

} // namespace

StartPageSettings::StartPageSettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
    if (value(RetiredHomePageKey).isValid()) {
        remove(RetiredHomePageKey);
    }
}

bool StartPageSettings::blank() const
{
    return flag(BlankKey, false);
}

void StartPageSettings::setBlank(bool blank)
{
    if (setFlag(BlankKey, blank, false)) {
        emit changed();
    }
}

bool StartPageSettings::topSites() const
{
    return flag(TopSitesKey);
}

void StartPageSettings::setTopSites(bool shown)
{
    if (setFlag(TopSitesKey, shown)) {
        emit changed();
    }
}

bool StartPageSettings::bookmarks() const
{
    return flag(BookmarksKey);
}

void StartPageSettings::setBookmarks(bool shown)
{
    if (setFlag(BookmarksKey, shown)) {
        emit changed();
    }
}

bool StartPageSettings::recent() const
{
    return flag(RecentKey);
}

void StartPageSettings::setRecent(bool shown)
{
    if (setFlag(RecentKey, shown)) {
        emit changed();
    }
}

} // namespace Salama
