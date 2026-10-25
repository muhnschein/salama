// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/ExitWatchdog.h"

#include <QGuiApplication>
#include <QSignalSpy>
#include <QtTest>

using Salama::ExitWatchdog;

namespace {

// Short to wait out; "nothing yet" waits are few timeouts long.
const int Timeout = 20;

// Qt emits it only inside exec(), which test never enters.
void closeLastWindow()
{
    emit qGuiApp->lastWindowClosed();
}

} // namespace

class tst_exitwatchdog : public QObject
{
    Q_OBJECT

private slots:
    void waitsPastPluginShutdown();
    void idleWhileWindowOpen();
    void expiresAfterLastWindowClosed();
    void expiresOnce();
};

void tst_exitwatchdog::waitsPastPluginShutdown()
{
    // Earlier quit would cut short plugin's own wait for engine.
    QVERIFY(ExitWatchdog::DefaultTimeout > ExitWatchdog::PluginShutdownLimit);
}

void tst_exitwatchdog::idleWhileWindowOpen()
{
    ExitWatchdog watchdog(qGuiApp, Timeout);
    QSignalSpy expired(&watchdog, &ExitWatchdog::expired);
    QVERIFY(!expired.wait(Timeout * 5));
}

void tst_exitwatchdog::expiresAfterLastWindowClosed()
{
    ExitWatchdog watchdog(qGuiApp, Timeout);
    QSignalSpy expired(&watchdog, &ExitWatchdog::expired);
    closeLastWindow();
    // Plugin gets its turn first.
    QCOMPARE(expired.count(), 0);
    QVERIFY(expired.wait());
    QCOMPARE(expired.count(), 1);
}

void tst_exitwatchdog::expiresOnce()
{
    ExitWatchdog watchdog(qGuiApp, Timeout);
    QSignalSpy expired(&watchdog, &ExitWatchdog::expired);
    closeLastWindow();
    QVERIFY(expired.wait());
    QVERIFY(!expired.wait(Timeout * 5));
    QCOMPARE(expired.count(), 1);
}

QTEST_MAIN(tst_exitwatchdog)
#include "tst_exitwatchdog.moc"
