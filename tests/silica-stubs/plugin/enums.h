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

// What a text field does with its focus when a press lands outside it: Silica's TextBase
// clears the field's own focus, the page's, or keeps it. Silica's plugin source is not
// among what Jolla publishes, so the values follow the order Silica documents them in;
// QML reads them by name.
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

// How a page is put on the stack or taken off it: with Silica's transition, or at once.
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

// The movement a TouchInteractionHint shows: which way the finger goes, and whether it
// swipes, swipes in from the edge of the screen, or pulls. The values are Silica's own,
// as its plugins.qmltypes lists them.
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
