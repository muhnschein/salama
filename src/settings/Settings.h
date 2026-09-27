// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

// What belongs to no settings page of its own: the website colours and the screen
// cutout's switch, under Appearance on the main page, and whether the tutorial has been
// shown. The pages' own are SearchSettings, ReaderSettings, CoverSettings,
// PrivacySettings and StartPageSettings (docs/DECISIONS/0028-settings-pages.md).
class Settings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(bool cutoutGuard READ cutoutGuard WRITE setCutoutGuard NOTIFY cutoutGuardChanged)
    // Whether pages are drawn in the colours they keep for a dark screen or a light one,
    // a WebsiteColors value: Firefox's Website appearance
    // (docs/DECISIONS/0035-website-colours.md).
    Q_PROPERTY(
        int websiteColors READ websiteColors WRITE setWebsiteColors NOTIFY websiteColorsChanged)
    // Whether the tutorial has been shown: it comes up by itself over the browsing page
    // until it has been, once, and Settings > Tutorial shows it again whenever asked
    // (docs/DECISIONS/0034-tutorial.md).
    Q_PROPERTY(
        bool tutorialShown READ tutorialShown WRITE setTutorialShown NOTIFY tutorialShownChanged)

public:
    // What a page is told the screen is, as the CSS prefers-color-scheme it reads: dark
    // or light as the ambience is, or light, or dark, whatever the ambience. Firefox's
    // Automatic, Light and Dark, in its order. Stored, so the numbers are part of the
    // file format, and unscoped as ReaderSettings::Colors is: QML reads
    // `Settings.WebsiteColorsDark`.
    enum WebsiteColors // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        WebsiteColorsAutomatic = 0,
        WebsiteColorsLight = 1,
        WebsiteColorsDark = 2
    };
    Q_ENUM(WebsiteColors)

    explicit Settings(QSettings &file, QObject *parent = nullptr);

    // Whether this application keeps out of the display's own cutout. On by default:
    // a camera notch over the first line of a page is not a design decision.
    bool cutoutGuard() const;
    void setCutoutGuard(bool cutoutGuard);

    // Automatic unless changed. Out of range reads back as the default.
    int websiteColors() const;
    void setWebsiteColors(int colors);

    // Off until the tutorial first comes up.
    bool tutorialShown() const;
    void setTutorialShown(bool shown);

    // How many screen pixels the engine lays a css pixel out on, for a screen of this
    // Theme.pixelRatio: 1.75 of it in steps of a half, which is about 360 css pixels
    // across a 1080 wide screen -- the width a phone layout is written for -- where the
    // platform's own 1.5 gives 410. The browsing page hands it the engine; the reader
    // settings' preview sets its text by it, as large as the reader view will.
    Q_INVOKABLE static qreal pageZoom(qreal pixelRatio);

signals:
    void cutoutGuardChanged();
    void websiteColorsChanged();
    void tutorialShownChanged();
};

} // namespace Salama
