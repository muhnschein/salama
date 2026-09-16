// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#include "storage/Storage.h"
#include "tabs/TabPersistence.h"

#include <QTemporaryDir>
#include <QtTest>

using Tuuli::Storage;
using Tuuli::Tab;
using Tuuli::TabPersistence;

class tst_tabpersistence : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip();
    void saveOrderRenumbers();
    void ignoresPrivateAndInvalidTabs();
    void activeTabId();
    void removeAll();
};

namespace {

Tab makeTab(int id, const QString &url, bool isPrivate = false)
{
    Tab tab;
    tab.id = id;
    tab.url = url;
    tab.title = QStringLiteral("Title %1").arg(id);
    tab.lastActive = id;
    tab.isPrivate = isPrivate;
    return tab;
}

} // namespace

void tst_tabpersistence::roundTrip()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);

    QVERIFY(persistence.loadTabs().isEmpty());
    persistence.insertTab(makeTab(7, QStringLiteral("https://a.example/")));
    persistence.insertTab(makeTab(3, QStringLiteral("https://b.example/")));

    QList<Tab> tabs = persistence.loadTabs();
    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.at(0).id, 7);
    QCOMPARE(tabs.at(1).id, 3);
    QCOMPARE(tabs.at(1).title, QStringLiteral("Title 3"));
    // The cover's order is written with the rest of the tab, not derived on load.
    QCOMPARE(tabs.at(0).lastActive, 7LL);

    Tab updated = tabs.at(0);
    updated.url = QStringLiteral("https://a.example/page");
    updated.favicon = QStringLiteral("https://a.example/favicon.ico");
    updated.lastActive = 99;
    persistence.updateTab(updated);
    tabs = persistence.loadTabs();
    QCOMPARE(tabs.at(0), updated);
    QVERIFY(tabs.at(0) != tabs.at(1));

    persistence.removeTab(7);
    tabs = persistence.loadTabs();
    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.first().id, 3);
}

void tst_tabpersistence::saveOrderRenumbers()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    persistence.insertTab(makeTab(1, QStringLiteral("https://a.example/")));
    persistence.insertTab(makeTab(2, QStringLiteral("https://b.example/")));
    persistence.insertTab(makeTab(3, QStringLiteral("https://c.example/")));

    QList<Tab> tabs = persistence.loadTabs();
    tabs.move(0, 2);
    // A private tab in the middle of the list has no row of its own and must not
    // consume a position or upset the ones around it.
    tabs.insert(1, makeTab(9, QStringLiteral("https://secret.example/"), true));
    persistence.saveOrder(tabs);

    const QList<Tab> reloaded = persistence.loadTabs();
    QCOMPARE(reloaded.count(), 3);
    QCOMPARE(reloaded.at(0).id, 2);
    QCOMPARE(reloaded.at(1).id, 3);
    QCOMPARE(reloaded.at(2).id, 1);

    // Positions were renumbered from 1, so a new tab still lands after all of them.
    persistence.insertTab(makeTab(4, QStringLiteral("https://d.example/")));
    QCOMPARE(persistence.loadTabs().at(3).id, 4);
}

void tst_tabpersistence::ignoresPrivateAndInvalidTabs()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);

    persistence.insertTab(makeTab(1, QStringLiteral("https://secret.example/"), true));
    persistence.insertTab(makeTab(0, QStringLiteral("https://invalid.example/")));
    QVERIFY(persistence.loadTabs().isEmpty());

    persistence.insertTab(makeTab(2, QStringLiteral("https://public.example/")));
    Tab privateUpdate = makeTab(2, QStringLiteral("https://changed.example/"), true);
    persistence.updateTab(privateUpdate);
    QCOMPARE(persistence.loadTabs().first().url, QStringLiteral("https://public.example/"));
}

void tst_tabpersistence::activeTabId()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);

    QCOMPARE(persistence.loadActiveTabId(), 0);
    persistence.setActiveTabId(5);
    QCOMPARE(persistence.loadActiveTabId(), 5);
    persistence.setActiveTabId(9);
    QCOMPARE(persistence.loadActiveTabId(), 9);
}

void tst_tabpersistence::removeAll()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);

    persistence.insertTab(makeTab(1, QStringLiteral("https://a.example/")));
    persistence.insertTab(makeTab(2, QStringLiteral("https://b.example/")));
    persistence.removeAllTabs();
    QVERIFY(persistence.loadTabs().isEmpty());
}

QTEST_GUILESS_MAIN(tst_tabpersistence)
#include "tst_tabpersistence.moc"
