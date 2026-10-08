// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

class Settings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(int notchGuard READ notchGuard WRITE setNotchGuard NOTIFY notchGuardChanged)
    // Guard not Disabled: tab grid head row + tutorial sketch avoid cutout.
    Q_PROPERTY(bool cutoutGuard READ cutoutGuard NOTIFY notchGuardChanged)
    Q_PROPERTY(bool fixedToolbar READ fixedToolbar WRITE setFixedToolbar NOTIFY fixedToolbarChanged)
    Q_PROPERTY(
        int websiteColors READ websiteColors WRITE setWebsiteColors NOTIFY websiteColorsChanged)
    Q_PROPERTY(
        bool tutorialShown READ tutorialShown WRITE setTutorialShown NOTIFY tutorialShownChanged)
    Q_PROPERTY(bool linkPreview READ linkPreview WRITE setLinkPreview NOTIFY linkPreviewChanged)

public:
    // Stored: file format.
    enum WebsiteColors // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        WebsiteColorsAutomatic = 0,
        WebsiteColorsLight = 1,
        WebsiteColorsDark = 2
    };
    Q_ENUM(WebsiteColors)

    // Automatic: only viewport-fit=cover pages get cutout. Stored: file format.
    enum NotchGuard // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        NotchGuardAutomatic = 0,
        NotchGuardForced = 1,
        NotchGuardDisabled = 2
    };
    Q_ENUM(NotchGuard)

    explicit Settings(QSettings &file, QObject *parent = nullptr);

    // Out of range -> default.
    int notchGuard() const;
    void setNotchGuard(int guard);
    bool cutoutGuard() const;

    bool fixedToolbar() const;
    void setFixedToolbar(bool fixed);

    int websiteColors() const;
    void setWebsiteColors(int colors);

    bool tutorialShown() const;
    void setTutorialShown(bool shown);

    bool linkPreview() const;
    void setLinkPreview(bool shown);

    // 1.75 x pixelRatio, 0.5 steps: ~360 CSS px on 1080 wide (phone layout width).
    Q_INVOKABLE static qreal pageZoom(qreal pixelRatio);

signals:
    void notchGuardChanged();
    void fixedToolbarChanged();
    void websiteColorsChanged();
    void tutorialShownChanged();
    void linkPreviewChanged();
};

} // namespace Salama
