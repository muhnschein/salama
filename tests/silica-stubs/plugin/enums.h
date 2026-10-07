// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Enum holders with Silica values. QML property names can't start upper-case, so
// Orientation.Portrait etc. must come from C++.
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

// First four: opaque -> clear. Last two: middle out to both edges.
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

// Focus on press outside field (Silica TextBase). Silica plugin source unpublished: values
// follow documented order; QML reads by name.
class FocusBehavior : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        ClearItemFocus = 0,
        ClearPageFocus = 1,
        KeepFocus = 2
    };
    Q_ENUM(Value)
};

class PageStackAction : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        Animated = 0,
        Immediate = 1
    };
    Q_ENUM(Value)
};

// Values from Silica plugins.qmltypes.
class TouchInteraction : public QObject
{
    Q_OBJECT

public:
    enum Direction
    {
        Left = 0,
        Up = 1,
        Right = 2,
        Down = 3
    };
    Q_ENUM(Direction)

    enum Mode
    {
        Swipe = 0,
        EdgeSwipe = 1,
        Pull = 2
    };
    Q_ENUM(Mode)
};
