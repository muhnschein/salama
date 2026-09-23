// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QVariant>

namespace Salama {

// Reading the numbers in what the engine says. They arrive in a QVariant, as a type
// that depends on who read them: qtmozembed reads the engine's JSON with
// QJsonDocument::toVariant() (src/qmozcontext.cpp), which gives every number as a
// double on the device's Qt 5.6, and a whole one as a qlonglong from Qt 5.15 on;
// handed over from QML, a whole number is an int. They are asked by type, because
// QVariant would read a number out of a string, or out of false, and would round a
// fraction to a whole one -- and the engine sends none of those.
namespace EngineData {

// A number of one of the types above, or of their unsigned kin: nothing is known to
// hand one of those over, but it would be a number all the same.
bool isNumber(const QVariant &value);

// The value as an id the way the engine counts them, from 1 up, or 0 when it is not
// one: not a number, not a whole one, or past what an int holds.
int id(const QVariant &value);

} // namespace EngineData

} // namespace Salama
