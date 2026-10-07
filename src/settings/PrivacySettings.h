// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

class PrivacySettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(int trackingProtection READ trackingProtection WRITE setTrackingProtection NOTIFY
                   trackingProtectionChanged)
    Q_PROPERTY(bool rememberHistory READ rememberHistory WRITE setRememberHistory NOTIFY
                   rememberHistoryChanged)
    Q_PROPERTY(bool clearHistoryOnClose READ clearHistoryOnClose WRITE setClearHistoryOnClose NOTIFY
                   clearHistoryOnCloseChanged)
    Q_PROPERTY(bool blockNotificationRequests READ blockNotificationRequests WRITE
                   setBlockNotificationRequests NOTIFY blockNotificationRequestsChanged)
    Q_PROPERTY(bool globalPrivacyControl READ globalPrivacyControl WRITE setGlobalPrivacyControl
                   NOTIFY globalPrivacyControlChanged)
    Q_PROPERTY(bool javascript READ javascript WRITE setJavascript NOTIFY javascriptChanged)
    // Off still tries HTTPS first.
    Q_PROPERTY(bool httpsOnly READ httpsOnly WRITE setHttpsOnly NOTIFY httpsOnlyChanged)

public:
    // Stored: values are file format.
    enum TrackingProtection // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        TrackingProtectionOff = 0,
        TrackingProtectionStandard = 1,
        TrackingProtectionStrict = 2
    };
    Q_ENUM(TrackingProtection)

    // Migrates old Do not track value to GPC.
    explicit PrivacySettings(QSettings &file, QObject *parent = nullptr);

    // Out of range -> default.
    int trackingProtection() const;
    void setTrackingProtection(int level);

    bool rememberHistory() const;
    void setRememberHistory(bool on);
    bool clearHistoryOnClose() const;
    void setClearHistoryOnClose(bool on);

    bool blockNotificationRequests() const;
    void setBlockNotificationRequests(bool on);

    bool globalPrivacyControl() const;
    void setGlobalPrivacyControl(bool on);
    bool javascript() const;
    void setJavascript(bool on);

    bool httpsOnly() const;
    void setHttpsOnly(bool on);

signals:
    void trackingProtectionChanged();
    void rememberHistoryChanged();
    void clearHistoryOnCloseChanged();
    void blockNotificationRequestsChanged();
    void globalPrivacyControlChanged();
    void javascriptChanged();
    void httpsOnlyChanged();
};

} // namespace Salama
