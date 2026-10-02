// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

// What belongs to no settings page of its own: the website colours, the notch guard and
// the fixed toolbar, under Appearance on the main page, whether the tutorial has been
// shown, and whether the link sheet shows a link's page. The pages' own are SearchSettings,
// ReaderSettings, CoverSettings, PrivacySettings and StartPageSettings
// (docs/DECISIONS/0028-settings-pages.md).
class Settings : public SettingsSection
{
    Q_OBJECT
    // How pages keep out of the display's cutout, a NotchGuard value: sailfish-browser's
    // three, in its order (docs/DECISIONS/0013-screen-cutout.md).
    Q_PROPERTY(int notchGuard READ notchGuard WRITE setNotchGuard NOTIFY notchGuardChanged)
    // Whether anything is kept out of the cutout at all: the tab grid's head row and the
    // tutorial's sketch are, unless the guard is disabled.
    Q_PROPERTY(bool cutoutGuard READ cutoutGuard NOTIFY notchGuardChanged)
    // Whether the navigation bar stays whole while a page is scrolled, rather than
    // slimming to its handle and the host: sailfish-browser's Fixed toolbar
    // (docs/DECISIONS/0009-navigation-bar-gesture.md).
    Q_PROPERTY(bool fixedToolbar READ fixedToolbar WRITE setFixedToolbar NOTIFY fixedToolbarChanged)
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
    // Whether the link sheet shows the page a link leads to, as Safari's link preview does:
    // the sheet's Show preview and Hide preview say it for every link from then on
    // (docs/DECISIONS/0046-link-menu.md).
    Q_PROPERTY(bool linkPreview READ linkPreview WRITE setLinkPreview NOTIFY linkPreviewChanged)

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

    // sailfish-browser's notch guard modes, in its menu's order: a page that asks for
    // the cutout with viewport-fit=cover gets it and every other page is kept below it;
    // every page is kept below it; no page is. Stored, so the numbers are part of the
    // file format.
    enum NotchGuard // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        NotchGuardAutomatic = 0,
        NotchGuardForced = 1,
        NotchGuardDisabled = 2
    };
    Q_ENUM(NotchGuard)

    explicit Settings(QSettings &file, QObject *parent = nullptr);

    // Automatic unless changed, as sailfish-browser's. A file from before there were
    // three is read as the switch it kept said: Forced for on, Disabled for off. Out of
    // range reads back as the default.
    int notchGuard() const;
    void setNotchGuard(int guard);
    bool cutoutGuard() const;

    // Off unless switched on.
    bool fixedToolbar() const;
    void setFixedToolbar(bool fixed);

    // Automatic unless changed. Out of range reads back as the default.
    int websiteColors() const;
    void setWebsiteColors(int colors);

    // Off until the tutorial first comes up.
    bool tutorialShown() const;
    void setTutorialShown(bool shown);

    // On unless switched off, as Safari's is.
    bool linkPreview() const;
    void setLinkPreview(bool shown);

    // How many screen pixels the engine lays a css pixel out on, for a screen of this
    // Theme.pixelRatio: 1.75 of it in steps of a half, which is about 360 css pixels
    // across a 1080 wide screen -- the width a phone layout is written for -- where the
    // platform's own 1.5 gives 410. The browsing page hands it the engine; the reader
    // settings' preview sets its text by it, as large as the reader view will.
    Q_INVOKABLE static qreal pageZoom(qreal pixelRatio);

signals:
    void notchGuardChanged();
    void fixedToolbarChanged();
    void websiteColorsChanged();
    void tutorialShownChanged();
    void linkPreviewChanged();
};

} // namespace Salama
