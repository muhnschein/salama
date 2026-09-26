// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

// The pages under Privacy in Settings: tracking protection, what is kept of the
// history, and whether sites may ask to send notifications.
class PrivacySettings : public SettingsSection
{
    Q_OBJECT
    // How much of the engine's own anti-tracking is switched on; a TrackingProtection
    // value (docs/DECISIONS/0023-tracking-protection.md).
    Q_PROPERTY(int trackingProtection READ trackingProtection WRITE setTrackingProtection NOTIFY
                   trackingProtectionChanged)
    // Whether the pages visited are kept in the history, on unless switched off, and
    // whether the history is cleared as the browser closes, off unless switched on:
    // Firefox's Remember browsing and download history and Clear history when Firefox
    // closes (docs/DECISIONS/0030-history-settings.md).
    Q_PROPERTY(bool rememberHistory READ rememberHistory WRITE setRememberHistory NOTIFY
                   rememberHistoryChanged)
    Q_PROPERTY(bool clearHistoryOnClose READ clearHistoryOnClose WRITE setClearHistoryOnClose NOTIFY
                   clearHistoryOnCloseChanged)
    // Whether sites the reader has not decided on may ask to send notifications, as they
    // may unless this is switched on: Firefox's Block new requests asking to allow
    // notifications (docs/DECISIONS/0033-web-notifications.md).
    Q_PROPERTY(bool blockNotificationRequests READ blockNotificationRequests WRITE
                   setBlockNotificationRequests NOTIFY blockNotificationRequestsChanged)

public:
    // Firefox's Enhanced Tracking Protection categories, less protection first, with
    // Off in place of Custom. Stored, so the numbers are part of the file format, and
    // unscoped as ReaderSettings::Colors is: QML reads
    // `PrivacySettings.TrackingProtectionOff`. What each asks of the engine is
    // EngineMessages::trackingProtectionPreferences().
    enum TrackingProtection // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        TrackingProtectionOff = 0,
        TrackingProtectionStandard = 1,
        TrackingProtectionStrict = 2
    };
    Q_ENUM(TrackingProtection)

    explicit PrivacySettings(QSettings &file, QObject *parent = nullptr);

    // Standard unless changed, as in Firefox. Out of range reads back as the default.
    int trackingProtection() const;
    void setTrackingProtection(int level);

    bool rememberHistory() const;
    void setRememberHistory(bool on);
    bool clearHistoryOnClose() const;
    void setClearHistoryOnClose(bool on);

    bool blockNotificationRequests() const;
    void setBlockNotificationRequests(bool on);

signals:
    void trackingProtectionChanged();
    void rememberHistoryChanged();
    void clearHistoryOnCloseChanged();
    void blockNotificationRequestsChanged();
};

} // namespace Salama
