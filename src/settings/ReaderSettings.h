// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

// Settings > Reader view: how the reader view sets an article
// (docs/DECISIONS/0024-reader-view.md). Out-of-range values read back as the defaults.
class ReaderSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(int colors READ colors WRITE setColors NOTIFY colorsChanged)
    Q_PROPERTY(int typeface READ typeface WRITE setTypeface NOTIFY typefaceChanged)
    Q_PROPERTY(int textSize READ textSize WRITE setTextSize NOTIFY textSizeChanged)

public:
    // The reader view's colours: the ambience's own, light or dark as it is, or one of
    // Firefox's reader themes whatever the ambience. Stored, so the numbers are part of
    // the file format.
    //
    // Unscoped on purpose, as every enum QML reads is (cpp:S3642): the page reaches
    // these as `ReaderSettings.Sepia`, and QML could not do that with a scoped enum
    // until Qt 5.8. This application is built against 5.6 (SCOPE.md §4), so `enum
    // class` here would compile on the host and leave the choice unset on the phone.
    enum Colors // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Ambience = 0,
        Light = 1,
        Sepia = 2,
        Dark = 3
    };
    Q_ENUM(Colors)

    // The reader view's typeface, as Firefox offers it. Stored, and unscoped as Colors
    // is.
    enum Typeface // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        SansSerif = 0,
        Serif = 1
    };
    Q_ENUM(Typeface)

    // The reader view's text size, in Firefox's reader.font_size steps: 1 to 9, 5 the
    // default. An enum so the slider in Settings can read its ends from here, and
    // unscoped as Colors is.
    enum TextSize // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        TextSizeMin = 1,
        TextSizeDefault = 5,
        TextSizeMax = 9
    };
    Q_ENUM(TextSize)

    explicit ReaderSettings(QSettings &file, QObject *parent = nullptr);

    int colors() const;
    void setColors(int colors);
    int typeface() const;
    void setTypeface(int typeface);
    int textSize() const;
    void setTextSize(int size);

signals:
    void colorsChanged();
    void typefaceChanged();
    void textSizeChanged();
};

} // namespace Salama
