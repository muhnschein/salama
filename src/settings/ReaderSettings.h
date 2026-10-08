// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

namespace Salama {

class ReaderSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(int colors READ colors WRITE setColors NOTIFY colorsChanged)
    Q_PROPERTY(int typeface READ typeface WRITE setTypeface NOTIFY typefaceChanged)
    Q_PROPERTY(int textSize READ textSize WRITE setTextSize NOTIFY textSizeChanged)

public:
    // Stored: values are file format.
    // Unscoped: QML reads scoped enums only from Qt 5.8; target 5.6 (SCOPE.md §4). `enum class`
    // compiles on host but leaves choice unset on phone.
    enum Colors // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Automatic = 0,
        Light = 1,
        Sepia = 2,
        Dark = 3,
        Ambience = 4
    };
    Q_ENUM(Colors)

    enum Typeface // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        SansSerif = 0,
        Serif = 1
    };
    Q_ENUM(Typeface)

    // Firefox reader.font_size steps.
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
