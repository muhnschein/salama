// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

// Defaults; exceptions in SitePermissions, notification default in PrivacySettings.
class SitePermissionSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(
        bool popupsAllowed READ popupsAllowed WRITE setPopupsAllowed NOTIFY popupsAllowedChanged)
    // false = Ask.
    Q_PROPERTY(bool locationBlocked READ locationBlocked WRITE setLocationBlocked NOTIFY
                   locationBlockedChanged)
    Q_PROPERTY(
        bool cameraBlocked READ cameraBlocked WRITE setCameraBlocked NOTIFY cameraBlockedChanged)
    Q_PROPERTY(bool microphoneBlocked READ microphoneBlocked WRITE setMicrophoneBlocked NOTIFY
                   microphoneBlockedChanged)
    // Applies only with tracking protection off; else its level decides.
    Q_PROPERTY(int cookies READ cookies WRITE setCookies NOTIFY cookiesChanged)

public:
    // Values = network.cookie.cookieBehavior. Stored: file format.
    enum Cookies // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        CookiesAllowAll = 0,
        CookiesBlockCrossSite = 1,
        CookiesBlockAll = 2
    };
    Q_ENUM(Cookies)

    explicit SitePermissionSettings(QSettings &file, QObject *parent = nullptr);

    bool popupsAllowed() const;
    void setPopupsAllowed(bool on);

    bool locationBlocked() const;
    void setLocationBlocked(bool on);
    bool cameraBlocked() const;
    void setCameraBlocked(bool on);
    bool microphoneBlocked() const;
    void setMicrophoneBlocked(bool on);

    // Out of range -> default.
    int cookies() const;
    void setCookies(int chosen);

signals:
    void popupsAllowedChanged();
    void locationBlockedChanged();
    void cameraBlockedChanged();
    void microphoneBlockedChanged();
    void cookiesChanged();
};

} // namespace Salama
