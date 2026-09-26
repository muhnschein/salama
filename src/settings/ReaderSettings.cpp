// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "ReaderSettings.h"

namespace Salama {

namespace {

const char *const ColorsKey = "readerColors";
const char *const TypefaceKey = "readerTypeface";
const char *const TextSizeKey = "readerTextSize";

} // namespace

ReaderSettings::ReaderSettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
}

int ReaderSettings::colors() const
{
    return choice(ColorsKey, Ambience, Ambience, Dark);
}

void ReaderSettings::setColors(int colors)
{
    if (setChoice(ColorsKey, colors, Ambience, Ambience, Dark)) {
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
