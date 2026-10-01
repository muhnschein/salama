// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SitePermissionSettings.h"

namespace Salama {

namespace {

const char *const PopupsAllowedKey = "popupsAllowed";
const char *const LocationBlockedKey = "locationBlocked";
const char *const CameraBlockedKey = "cameraBlocked";
const char *const MicrophoneBlockedKey = "microphoneBlocked";
const char *const CookiesKey = "cookies";

} // namespace

SitePermissionSettings::SitePermissionSettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
}

bool SitePermissionSettings::popupsAllowed() const
{
    return flag(PopupsAllowedKey, false);
}

void SitePermissionSettings::setPopupsAllowed(bool on)
{
    if (setFlag(PopupsAllowedKey, on, false)) {
        emit popupsAllowedChanged();
    }
}

bool SitePermissionSettings::locationBlocked() const
{
    return flag(LocationBlockedKey, false);
}

void SitePermissionSettings::setLocationBlocked(bool on)
{
    if (setFlag(LocationBlockedKey, on, false)) {
        emit locationBlockedChanged();
    }
}

bool SitePermissionSettings::cameraBlocked() const
{
    return flag(CameraBlockedKey, false);
}

void SitePermissionSettings::setCameraBlocked(bool on)
{
    if (setFlag(CameraBlockedKey, on, false)) {
        emit cameraBlockedChanged();
    }
}

bool SitePermissionSettings::microphoneBlocked() const
{
    return flag(MicrophoneBlockedKey, false);
}

void SitePermissionSettings::setMicrophoneBlocked(bool on)
{
    if (setFlag(MicrophoneBlockedKey, on, false)) {
        emit microphoneBlockedChanged();
    }
}

int SitePermissionSettings::cookies() const
{
    return choice(CookiesKey, CookiesBlockCrossSite, CookiesAllowAll, CookiesBlockAll);
}

void SitePermissionSettings::setCookies(int chosen)
{
    if (setChoice(CookiesKey, chosen, CookiesBlockCrossSite, CookiesAllowAll, CookiesBlockAll)) {
        emit cookiesChanged();
    }
}

} // namespace Salama
