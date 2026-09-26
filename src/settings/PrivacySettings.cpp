// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "PrivacySettings.h"

namespace Salama {

namespace {

const char *const TrackingProtectionKey = "trackingProtection";
const char *const RememberHistoryKey = "rememberHistory";
const char *const ClearHistoryOnCloseKey = "clearHistoryOnClose";
const char *const BlockNotificationRequestsKey = "blockNotificationRequests";

} // namespace

PrivacySettings::PrivacySettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
}

int PrivacySettings::trackingProtection() const
{
    return choice(TrackingProtectionKey, TrackingProtectionStandard, TrackingProtectionOff,
                  TrackingProtectionStrict);
}

void PrivacySettings::setTrackingProtection(int level)
{
    if (setChoice(TrackingProtectionKey, level, TrackingProtectionStandard, TrackingProtectionOff,
                  TrackingProtectionStrict)) {
        emit trackingProtectionChanged();
    }
}

bool PrivacySettings::rememberHistory() const
{
    return flag(RememberHistoryKey);
}

void PrivacySettings::setRememberHistory(bool on)
{
    if (setFlag(RememberHistoryKey, on)) {
        emit rememberHistoryChanged();
    }
}

bool PrivacySettings::clearHistoryOnClose() const
{
    return flag(ClearHistoryOnCloseKey, false);
}

void PrivacySettings::setClearHistoryOnClose(bool on)
{
    if (setFlag(ClearHistoryOnCloseKey, on, false)) {
        emit clearHistoryOnCloseChanged();
    }
}

bool PrivacySettings::blockNotificationRequests() const
{
    return flag(BlockNotificationRequestsKey, false);
}

void PrivacySettings::setBlockNotificationRequests(bool on)
{
    if (setFlag(BlockNotificationRequestsKey, on, false)) {
        emit blockNotificationRequestsChanged();
    }
}

} // namespace Salama
