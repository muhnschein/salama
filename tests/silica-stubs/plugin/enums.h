// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Enum holders mirroring Silica's values. QML property names cannot start with an
// upper-case letter, so `Orientation.Portrait` and friends must come from C++.
#pragma once

#include <QObject>

class Orientation : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        None = 0,
        Portrait = 1,
        Landscape = 2,
        PortraitInverted = 4,
        LandscapeInverted = 8,
        PortraitMask = 5,
        LandscapeMask = 10,
        All = 15
    };
    Q_ENUM(Value)
};

class PageStatus : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        Inactive = 0,
        Activating = 1,
        Active = 2,
        Deactivating = 3
    };
    Q_ENUM(Value)
};

// Where a cover is in coming into view on the home screen, as a page's status says
// where it is on the stack.
class Cover : public QObject
{
    Q_OBJECT

public:
    enum Status
    {
        Inactive = 0,
        Activating = 1,
        Active = 2,
        Deactivating = 3
    };
    Q_ENUM(Status)
};

class Dock : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        Top = 1,
        Bottom = 2,
        Left = 4,
        Right = 8
    };
    Q_ENUM(Value)
};

// Which way OpacityRampEffect fades: the first four from opaque to clear, the last two
// from the middle out to both edges.
class OpacityRamp : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        LeftToRight = 0,
        RightToLeft = 1,
        TopToBottom = 2,
        BottomToTop = 3,
        BothSides = 4,
        BothEnds = 5
    };
    Q_ENUM(Value)
};

class TruncationMode : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        None = 0,
        Elide = 1,
        Fade = 2
    };
    Q_ENUM(Value)
};
