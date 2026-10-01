// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "Settings.h"

namespace Salama {

namespace {

const char *const NotchGuardKey = "notchGuard";
// The switch the notch guard was before it had three modes: on kept every page below the
// cutout, which is Forced, and off kept none, which is Disabled.
const char *const RetiredCutoutGuardKey = "cutoutGuard";
const char *const FixedToolbarKey = "fixedToolbar";
const char *const TutorialShownKey = "tutorialShown";
const char *const WebsiteColorsKey = "websiteColors";

} // namespace

Settings::Settings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
    const QVariant retired = value(RetiredCutoutGuardKey);
    if (retired.isValid()) {
        if (!value(NotchGuardKey).isValid()) {
            setValue(NotchGuardKey, retired.toBool() ? NotchGuardForced : NotchGuardDisabled);
        }
        remove(RetiredCutoutGuardKey);
    }
}

int Settings::notchGuard() const
{
    return choice(NotchGuardKey, NotchGuardAutomatic, NotchGuardAutomatic, NotchGuardDisabled);
}

void Settings::setNotchGuard(int guard)
{
    if (setChoice(NotchGuardKey, guard, NotchGuardAutomatic, NotchGuardAutomatic,
                  NotchGuardDisabled)) {
        emit notchGuardChanged();
    }
}

bool Settings::cutoutGuard() const
{
    return notchGuard() != NotchGuardDisabled;
}

bool Settings::fixedToolbar() const
{
    return flag(FixedToolbarKey, false);
}

void Settings::setFixedToolbar(bool fixed)
{
    if (setFlag(FixedToolbarKey, fixed, false)) {
        emit fixedToolbarChanged();
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
