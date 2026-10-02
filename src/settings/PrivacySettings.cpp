// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "PrivacySettings.h"

namespace Salama {

namespace {

const char *const TrackingProtectionKey = "trackingProtection";
const char *const RememberHistoryKey = "rememberHistory";
const char *const ClearHistoryOnCloseKey = "clearHistoryOnClose";
const char *const BlockNotificationRequestsKey = "blockNotificationRequests";
// Do not track's, read once to start Global Privacy Control as it was left, then removed.
const char *const DoNotTrackKey = "doNotTrack";
const char *const GlobalPrivacyControlKey = "globalPrivacyControl";
const char *const JavascriptKey = "javascript";
const char *const HttpsOnlyKey = "httpsOnly";

} // namespace

PrivacySettings::PrivacySettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
    const QVariant doNotTrack = value(DoNotTrackKey);
    if (doNotTrack.isValid()) {
        if (!value(GlobalPrivacyControlKey).isValid()) {
            setValue(GlobalPrivacyControlKey, doNotTrack.toBool());
        }
        remove(DoNotTrackKey);
    }
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

bool PrivacySettings::globalPrivacyControl() const
{
    return flag(GlobalPrivacyControlKey, false);
}

void PrivacySettings::setGlobalPrivacyControl(bool on)
{
    if (setFlag(GlobalPrivacyControlKey, on, false)) {
        emit globalPrivacyControlChanged();
    }
}

bool PrivacySettings::javascript() const
{
    return flag(JavascriptKey);
}

void PrivacySettings::setJavascript(bool on)
{
    if (setFlag(JavascriptKey, on)) {
        emit javascriptChanged();
    }
}

bool PrivacySettings::httpsOnly() const
{
    return flag(HttpsOnlyKey, false);
}

void PrivacySettings::setHttpsOnly(bool on)
{
    if (setFlag(HttpsOnlyKey, on, false)) {
        emit httpsOnlyChanged();
    }
}

} // namespace Salama
