// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "storage/Storage.h"
#include "tabs/TabPersistence.h"

#include <QTemporaryDir>
#include <QtTest>

using Salama::ClosedTab;
using Salama::Storage;
using Salama::Tab;
using Salama::TabGroup;
using Salama::TabPersistence;

class tst_tabpersistence : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip();
    void saveOrderRenumbers();
    void ignoresInvalidTabs();
    void activeTabId();
    void removeAll();
    void groupsRoundTrip();
    void closedTabsRoundTrip();
};

namespace {

Tab makeTab(int id, const QString &url)
{
    Tab tab;
    tab.id = id;
    tab.url = url;
    tab.title = QStringLiteral("Title %1").arg(id);
    tab.lastActive = id;
    tab.groupId = 1;
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
    QCOMPARE(tabs.at(0).groupId, 1);

    Tab updated = tabs.at(0);
    updated.url = QStringLiteral("https://a.example/page");
    updated.favicon = QStringLiteral("https://a.example/favicon.ico");
    updated.lastActive = 99;
    updated.groupId = 3;
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
    // An invalid tab in the middle of the list has no row of its own and must not
    // consume a position or upset the ones around it.
    tabs.insert(1, makeTab(0, QStringLiteral("https://nowhere.example/")));
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

void tst_tabpersistence::ignoresInvalidTabs()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);

    persistence.insertTab(makeTab(1, QStringLiteral("https://first.example/")));
    persistence.insertTab(makeTab(0, QStringLiteral("https://invalid.example/")));
    QList<Tab> tabs = persistence.loadTabs();
    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.first().url, QStringLiteral("https://first.example/"));

    persistence.insertTab(makeTab(2, QStringLiteral("https://public.example/")));
    persistence.updateTab(makeTab(2, QStringLiteral("https://changed.example/")));
    tabs = persistence.loadTabs();
    QCOMPARE(tabs.at(1).url, QStringLiteral("https://changed.example/"));
    persistence.updateTab(makeTab(0, QStringLiteral("https://invalid.example/")));
    QCOMPARE(persistence.loadTabs().count(), 2);
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

void tst_tabpersistence::groupsRoundTrip()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);

    QVERIFY(persistence.loadGroups().isEmpty());
    QCOMPARE(persistence.loadCurrentGroupId(), 0);

    TabGroup first;
    first.id = 5;
    TabGroup second;
    second.id = 2;
    second.name = QStringLiteral("Work");
    TabGroup invalid;
    persistence.insertGroup(first);
    persistence.insertGroup(second);
    persistence.insertGroup(invalid);
    persistence.updateGroup(invalid);

    // In the order they were added, not by id.
    QList<TabGroup> groups = persistence.loadGroups();
    QCOMPARE(groups.count(), 2);
    QCOMPARE(groups.at(0), first);
    QCOMPARE(groups.at(1), second);
    QVERIFY(groups.at(0) != groups.at(1));
    QVERIFY(groups.at(0).name.isEmpty());
    QCOMPARE(groups.at(1).name, QStringLiteral("Work"));

    // The order can be written back from a list.
    persistence.saveGroupOrder(QList<TabGroup>{second, invalid, first});
    groups = persistence.loadGroups();
    QCOMPARE(groups.at(0).id, 2);
    QCOMPARE(groups.at(1).id, 5);
    persistence.saveGroupOrder(QList<TabGroup>{first, second});

    second.name = QStringLiteral("Office");
    persistence.updateGroup(second);
    QCOMPARE(persistence.loadGroups().at(1).name, QStringLiteral("Office"));

    persistence.setCurrentGroupId(2);
    QCOMPARE(persistence.loadCurrentGroupId(), 2);

    persistence.removeGroup(5);
    groups = persistence.loadGroups();
    QCOMPARE(groups.count(), 1);
    QCOMPARE(groups.first().id, 2);
}

void tst_tabpersistence::closedTabsRoundTrip()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    QVERIFY(persistence.loadClosedTabs().isEmpty());

    ClosedTab older;
    older.id = 1;
    older.url = QStringLiteral("https://old.example/");
    older.title = QStringLiteral("Old");
    older.closedAt = 100;
    ClosedTab newer;
    newer.id = 2;
    newer.url = QStringLiteral("https://new.example/");
    newer.favicon = QStringLiteral("https://new.example/favicon.ico");
    newer.closedAt = 200;
    ClosedTab invalid;
    persistence.insertClosedTab(older);
    persistence.insertClosedTab(newer);
    persistence.insertClosedTab(invalid);

    // Newest first.
    QList<ClosedTab> closed = persistence.loadClosedTabs();
    QCOMPARE(closed.count(), 2);
    QCOMPARE(closed.at(0), newer);
    QCOMPARE(closed.at(1), older);
    QVERIFY(closed.at(0) != closed.at(1));

    persistence.removeClosedTab(2);
    closed = persistence.loadClosedTabs();
    QCOMPARE(closed.count(), 1);
    QCOMPARE(closed.first().id, 1);
    persistence.removeAllClosedTabs();
    QVERIFY(persistence.loadClosedTabs().isEmpty());
}

QTEST_GUILESS_MAIN(tst_tabpersistence)
#include "tst_tabpersistence.moc"
