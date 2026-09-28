// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "ReaderSettings.h"

namespace Salama {

namespace {

const char *const ColorsKey = "readerColorScheme";
// Where the colours were kept before the Ambience look was the default, when 0 was the
// light or dark theme as the ambience is -- all "Ambience" meant then.
const char *const RetiredColorsKey = "readerColors";
const char *const TypefaceKey = "readerTypeface";
const char *const TextSizeKey = "readerTextSize";

} // namespace

ReaderSettings::ReaderSettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
    // A reader who had the ambience followed has the Ambience look, which follows it
    // too; the others keep what they chose.
    const QVariant retired = value(RetiredColorsKey);
    if (retired.isValid()) {
        if (retired.toInt() != Automatic) {
            setValue(ColorsKey, retired);
        }
        remove(RetiredColorsKey);
    }
}

int ReaderSettings::colors() const
{
    return choice(ColorsKey, Ambience, Automatic, Ambience);
}

void ReaderSettings::setColors(int colors)
{
    if (setChoice(ColorsKey, colors, Ambience, Automatic, Ambience)) {
        emit colorsChanged();
    }
}

int ReaderSettings::typeface() const
{
    return choice(TypefaceKey, SansSerif, SansSerif, Serif);
}

void ReaderSettings::setTypeface(int typeface)
{
    if (setChoice(TypefaceKey, typeface, SansSerif, SansSerif, Serif)) {
        emit typefaceChanged();
    }
}

int ReaderSettings::textSize() const
{
    return choice(TextSizeKey, TextSizeDefault, TextSizeMin, TextSizeMax);
}

void ReaderSettings::setTextSize(int size)
{
    if (setChoice(TextSizeKey, size, TextSizeDefault, TextSizeMin, TextSizeMax)) {
        emit textSizeChanged();
    }
}

} // namespace Salama
