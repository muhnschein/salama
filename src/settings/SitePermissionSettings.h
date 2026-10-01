// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

// What sites may do unless the reader decided otherwise for a site: the defaults
// Settings > Site permissions shows (docs/DECISIONS/0039-site-permissions.md). The
// exceptions are the engine's and SitePermissions'; whether sites may ask to send
// notifications is PrivacySettings', where it was before this section was.
//
// What each stands for in the engine's preferences is
// EngineMessages::sitePermissionPreferences(), and the cookies' choice
// EngineMessages::trackingProtectionPreferences().
class SitePermissionSettings : public SettingsSection
{
    Q_OBJECT
    // Whether a page may open a window it was not asked to by a touch, as Firefox's
    // "Block pop-up windows" says the other way round: off unless switched on.
    Q_PROPERTY(
        bool popupsAllowed READ popupsAllowed WRITE setPopupsAllowed NOTIFY popupsAllowedChanged)
    // Whether a site is refused a permission it has not been given or refused for itself,
    // and not asked: Ask, which is Firefox's default and what these are, or Block.
    Q_PROPERTY(bool locationBlocked READ locationBlocked WRITE setLocationBlocked NOTIFY
                   locationBlockedChanged)
    Q_PROPERTY(
        bool cameraBlocked READ cameraBlocked WRITE setCameraBlocked NOTIFY cameraBlockedChanged)
    Q_PROPERTY(bool microphoneBlocked READ microphoneBlocked WRITE setMicrophoneBlocked NOTIFY
                   microphoneBlockedChanged)
    // What cookies are accepted while tracking protection is off; a Cookies value. With
    // tracking protection on, its level says (docs/DECISIONS/0023-tracking-protection.md).
    Q_PROPERTY(int cookies READ cookies WRITE setCookies NOTIFY cookiesChanged)

public:
    // Firefox's cookie choices, by the engine's own number for each
    // (network.cookie.cookieBehavior: BEHAVIOR_ACCEPT, BEHAVIOR_REJECT_FOREIGN,
    // BEHAVIOR_REJECT in netwerk/cookie/nsICookieService.idl). Stored, so the numbers are
    // part of the file format, and unscoped as PrivacySettings::TrackingProtection is:
    // QML reads `SitePermissionSettings.CookiesBlockAll`.
    enum Cookies // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        CookiesAllowAll = 0,
        CookiesBlockCrossSite = 1,
        CookiesBlockAll = 2
    };
    Q_ENUM(Cookies)

    explicit SitePermissionSettings(QSettings &file, QObject *parent = nullptr);

    // Pop-ups blocked unless changed, as Firefox has them.
    bool popupsAllowed() const;
    void setPopupsAllowed(bool on);

    // Asked unless changed.
    bool locationBlocked() const;
    void setLocationBlocked(bool on);
    bool cameraBlocked() const;
    void setCameraBlocked(bool on);
    bool microphoneBlocked() const;
    void setMicrophoneBlocked(bool on);

    // Cross-site cookies blocked unless changed, as Firefox's Standard has them. Out of
    // range reads back as the default.
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
