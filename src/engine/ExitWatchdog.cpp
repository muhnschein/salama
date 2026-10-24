// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "ExitWatchdog.h"

#include <QGuiApplication>

namespace Salama {

static_assert(ExitWatchdog::DefaultTimeout > ExitWatchdog::PluginShutdownLimit,
              "plugin must get its whole shutdown first");

ExitWatchdog::ExitWatchdog(QGuiApplication *app, int timeout, QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(timeout);
    connect(&m_timer, &QTimer::timeout, this, &ExitWatchdog::expired);
    // Armed only once window gone: no timer while app in use.
    connect(app, &QGuiApplication::lastWindowClosed, &m_timer,
            static_cast<void (QTimer::*)()>(&QTimer::start));
}

} // namespace Salama
