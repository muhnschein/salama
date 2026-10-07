// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Host stand-in for libsailfishapp header: src/main.cpp builds under -Werror/clang-tidy
// without SDK. Never installed.
#pragma once

#include <QString>
#include <QUrl>

class QGuiApplication;
class QQuickView;

namespace SailfishApp {

QGuiApplication *application(int &argc, char **argv);
QQuickView *createView();
QUrl pathTo(const QString &filename);
QUrl pathToMainQml();

} // namespace SailfishApp
