// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QTimer>

class QGuiApplication;

namespace Salama {

// Sailfish.WebView before 1.8.0: on last window close plugin turns off quitOnLastWindowClosed,
// quits once engine context destroyed. Engine slower than plugin's 5 s watchdog -> setting
// restored, quit never called. Windowless process stays in sailjail, keeps single-instance
// launch and D-Bus name: next start from app grid times out. Upstream fix:
// sailfish-components-webview 58f0631 "Allow application exit after WebEngine shutdown times
// out". Expires if app still runs past plugin's worst case; with fix, app gone first.
class ExitWatchdog : public QObject
{
    Q_OBJECT

public:
    // Plugin worst case: 2 s for views to drain, then 5 s for engine context.
    static constexpr int PluginShutdownLimit = 2000 + 5000;
    static constexpr int DefaultTimeout = PluginShutdownLimit + 1000;

    explicit ExitWatchdog(QGuiApplication *app, int timeout = DefaultTimeout,
                          QObject *parent = nullptr);

signals:
    // Connect to QCoreApplication::quit: plugin's aboutToQuit handler still waits for engine.
    void expired();

private:
    QTimer m_timer;
};

} // namespace Salama
