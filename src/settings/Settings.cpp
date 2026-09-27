// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Settings.h"

namespace Salama {

namespace {

const char *const CutoutGuardKey = "cutoutGuard";
const char *const TutorialShownKey = "tutorialShown";
const char *const WebsiteColorsKey = "websiteColors";

} // namespace

Settings::Settings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
}

bool Settings::cutoutGuard() const
{
    return flag(CutoutGuardKey);
}

void Settings::setCutoutGuard(bool cutoutGuard)
{
    if (setFlag(CutoutGuardKey, cutoutGuard)) {
        emit cutoutGuardChanged();
    }
}

int Settings::websiteColors() const
{
    return choice(WebsiteColorsKey, WebsiteColorsAutomatic, WebsiteColorsAutomatic,
                  WebsiteColorsDark);
}

void Settings::setWebsiteColors(int colors)
{
    if (setChoice(WebsiteColorsKey, colors, WebsiteColorsAutomatic, WebsiteColorsAutomatic,
                  WebsiteColorsDark)) {
        emit websiteColorsChanged();
    }
}

bool Settings::tutorialShown() const
{
    return flag(TutorialShownKey, false);
}

void Settings::setTutorialShown(bool shown)
{
    if (setFlag(TutorialShownKey, shown, false)) {
        emit tutorialShownChanged();
    }
}

qreal Settings::pageZoom(qreal pixelRatio)
{
    return qRound(pixelRatio * 1.75 / 0.5) * 0.5;
}

} // namespace Salama
