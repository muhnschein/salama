// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "storage/Storage.h"
#include "tabs/ClosedTabModel.h"
#include "tabs/GroupTabModel.h"
#include "tabs/TabGroupModel.h"
#include "tabs/TabModel.h"
#include "tabs/TabPersistence.h"
#include "tabs/TabSearchModel.h"
#include "tabs/ThumbnailWriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>
#include <algorithm>

using Salama::ClosedTabModel;
using Salama::GroupTabModel;
using Salama::Storage;
using Salama::Tab;
using Salama::TabGroup;
using Salama::TabGroupModel;
using Salama::TabModel;
using Salama::TabPersistence;
using Salama::TabSearchModel;

class tst_tabmodel : public QObject
{
    Q_OBJECT

private slots:
    void emptyModel();
    void newTabAppendsAndActivates();
    void newTabRejectsExternalUrls();
    void newTabBehindStaysBehind();
    void activation();
    void moveTabReorders();
    void closeTabActivatesPrevious();
    void closeAllTabs();
    void urlUpdatesAndVisits();
    void titleAndFavicon();
    void persistenceRoundTrip();
    void thumbnailsAreCapturedPerTab();
    void thumbnailsFollowTabLifetime();
    void thumbnailsAreOptional();
    void thumbnailsAreWrittenOffTheGuiThread();
    void staleThumbnailWritesAreDiscarded();
    void recentThumbnailsFollowTheFront();
    void recentOrderSurvivesARestart();
    void thereIsAlwaysAGroup();
    void groupsHoldTheirOwnTabs();
    void movingTabsInsideAGroup();
    void closingStaysInTheGroup();
    void removingAGroupClosesItsTabs();
    void movingATabToAnotherGroup();
    void groupsSurviveARestart();
    void ungroupingKeepsTheTabs();
    void movingGroups();
    void groupPreviews();
    void searchSpansTheGroups();
    void searchRefinesWithoutResetting();
    void searchTakesEveryWord();
    void tabIdForUrl();
    void closedTabsCanBeReopened();
    void livePagesAreCapped();
    void mediaFollowsThePage();
    void mediaMetadataGoesWithWhatPlays();
    void startPageTabs();
    void sharedLinksOpenInTheDefaultGroup();
};

namespace {

bool writeFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write("png");
    return true;
}

} // namespace

namespace {

QVariant role(const TabModel &model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

QVariant role(const QAbstractListModel &model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

QList<int> groupTabIds(const GroupTabModel &model)
{
    QList<int> ids;
    for (int row = 0; row < model.count(); ++row) {
        ids.append(model.tabIdAt(row));
    }
    return ids;
}

} // namespace

void tst_tabmodel::emptyModel()
{
    TabModel model(nullptr);
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);
    QCOMPARE(model.activeTabIndex(), -1);
    QCOMPARE(model.activeTabId(), 0);
    QVERIFY(model.activeUrl().isEmpty());
    QVERIFY(model.activeTitle().isEmpty());
    QVERIFY(model.activeFavicon().isEmpty());
    QVERIFY(!model.data(model.index(0, 0), roleId(TabModel::Role::Url)).isValid());
    QCOMPARE(model.roleNames().value(roleId(TabModel::Role::Active)),
             QByteArrayLiteral("activeTab"));
    model.activateTab(0);
    model.closeActiveTab();
    model.closeAllTabs();
    QCOMPARE(model.count(), 0);
}

void tst_tabmodel::newTabAppendsAndActivates()
{
    TabModel model(nullptr);
    QSignalSpy countSpy(&model, &TabModel::countChanged);
    QSignalSpy addedSpy(&model, &TabModel::tabAdded);
    QSignalSpy activeSpy(&model, &TabModel::activeTabChanged);
    QSignalSpy dataSpy(&model, &TabModel::activeTabDataChanged);

    const int first = model.newTab(QStringLiteral("https://one.example/"));
    const int second = model.newTab(QStringLiteral("https://two.example/"));
    QVERIFY(first > 0);
    QCOMPARE(second, first + 1);
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 2);
    QCOMPARE(addedSpy.count(), 2);
    QCOMPARE(addedSpy.last().first().toInt(), second);
    QCOMPARE(activeSpy.count(), 2);
    QCOMPARE(dataSpy.count(), 2);

    QCOMPARE(model.activeTabIndex(), 1);
    QCOMPARE(model.activeTabId(), second);
    QCOMPARE(model.activeUrl(), QStringLiteral("https://two.example/"));
    QCOMPARE(role(model, 0, roleId(TabModel::Role::TabId)).toInt(), first);
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Url)).toString(),
             QStringLiteral("https://one.example/"));
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Active)).toBool(), false);
    QCOMPARE(role(model, 1, roleId(TabModel::Role::Active)).toBool(), true);
    QVERIFY(role(model, 1, roleId(TabModel::Role::Title)).toString().isEmpty());
    QVERIFY(role(model, 1, roleId(TabModel::Role::Favicon)).toString().isEmpty());
    QVERIFY(!role(model, 1, Qt::DisplayRole).isValid());
    QCOMPARE(model.tabs().count(), 2);
}

void tst_tabmodel::newTabRejectsExternalUrls()
{
    TabModel model(nullptr);
    QCOMPARE(model.newTab(QStringLiteral("tel:+358401234567")), 0);
    QCOMPARE(model.newTab(QStringLiteral("sms:123")), 0);
    QCOMPARE(model.newTab(QStringLiteral("mailto:a@b.c")), 0);
    QCOMPARE(model.newTab(QStringLiteral("geo:60.17,24.94")), 0);
    QCOMPARE(model.count(), 0);
    QVERIFY(TabModel::isExternalUrl(QStringLiteral("MAILTO:x@y.z")));
    QVERIFY(!TabModel::isExternalUrl(QStringLiteral("https://x.y/")));
}

void tst_tabmodel::newTabBehindStaysBehind()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    int reading = 0;
    int front = 0;
    int behind = 0;
    {
        TabModel model(&persistence);
        const int first = model.newTab(QStringLiteral("https://one.example/"));
        reading = model.addGroup(QStringLiteral("Reading"));
        front = model.newTab(QStringLiteral("https://trails.example/"));
        QSignalSpy activeSpy(&model, &TabModel::activeTabChanged);
        QSignalSpy dataSpy(&model, &TabModel::activeTabDataChanged);
        QSignalSpy addedSpy(&model, &TabModel::tabAdded);
        QSignalSpy titleSpy(&model, &TabModel::titleUpdated);

        behind = model.newTabBehind(QStringLiteral("https://trails.example/ridge"),
                                    QStringLiteral("The ridge loop"));
        QVERIFY(behind > front);
        QCOMPARE(model.count(), 3);
        QCOMPARE(model.activeTabId(), front);
        QCOMPARE(model.currentGroupId(), reading);
        QCOMPARE(activeSpy.count(), 0);
        QCOMPARE(dataSpy.count(), 0);
        QCOMPARE(addedSpy.count(), 1);
        QCOMPARE(addedSpy.first().first().toInt(), behind);
        QCOMPARE(titleSpy.count(), 0);
        const int row = model.indexOf(behind);
        QCOMPARE(role(model, row, roleId(TabModel::Role::Title)).toString(),
                 QStringLiteral("The ridge loop"));
        QCOMPARE(role(model, row, roleId(TabModel::Role::Group)).toInt(), reading);
        QCOMPARE(role(model, row, roleId(TabModel::Role::Active)).toBool(), false);
        QCOMPARE(model.groupNameOf(behind), QStringLiteral("Reading"));
        QCOMPARE(model.groupNameOf(first), QString());
        QCOMPARE(model.groupNameOf(9999), QString());

        QCOMPARE(model.newTabBehind(QString(), QStringLiteral("Nothing")), 0);
        QCOMPARE(model.newTabBehind(QStringLiteral("mailto:a@b.c"), QStringLiteral("Mail")), 0);
        QCOMPARE(model.count(), 3);

        QSignalSpy visitedSpy(&model, &TabModel::visited);
        model.activateTabById(behind);
        model.updateUrl(behind, QStringLiteral("https://trails.example/ridge"));
        QCOMPARE(visitedSpy.count(), 1);
        model.activateTabById(front);
    }
    TabModel restored(&persistence);
    QCOMPARE(restored.count(), 3);
    QCOMPARE(restored.activeTabId(), front);
    QCOMPARE(role(restored, restored.indexOf(behind), roleId(TabModel::Role::Title)).toString(),
             QStringLiteral("The ridge loop"));
}

void tst_tabmodel::activation()
{
    TabModel model(nullptr);
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    const int b = model.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy activeSpy(&model, &TabModel::activeTabChanged);
    QSignalSpy rowSpy(&model, &TabModel::dataChanged);
    QSignalSpy leavingSpy(&model, &TabModel::activeTabLeaving);
    // Left tab named before rows report it's no longer front.
    int rowsWhenLeft = -1;
    connect(&model, &TabModel::activeTabLeaving, this,
            [&rowSpy, &rowsWhenLeft]() { rowsWhenLeft = rowSpy.count(); });

    model.activateTab(0);
    QCOMPARE(model.activeTabId(), a);
    QCOMPARE(activeSpy.count(), 1);
    QCOMPARE(rowSpy.count(), 2);
    QCOMPARE(leavingSpy.count(), 1);
    QCOMPARE(leavingSpy.first().at(0).toInt(), b);
    QCOMPARE(rowsWhenLeft, 0);

    model.activateTab(0);
    QCOMPARE(activeSpy.count(), 1);
    QCOMPARE(leavingSpy.count(), 1);

    model.activateTab(99);
    QCOMPARE(model.activeTabId(), b);
    model.activateTab(-5);
    QCOMPARE(model.activeTabId(), a);

    QVERIFY(model.activateTabById(b));
    QCOMPARE(model.activeTabIndex(), 1);
    QVERIFY(!model.activateTabById(1234));
    QCOMPARE(model.activeTabIndex(), 1);
    QCOMPARE(model.indexOf(a), 0);
    QCOMPARE(model.indexOf(1234), -1);
}

void tst_tabmodel::moveTabReorders()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    TabModel model(&persistence);
    const int first = model.newTab(QStringLiteral("https://a.example/"));
    const int second = model.newTab(QStringLiteral("https://b.example/"));
    const int third = model.newTab(QStringLiteral("https://c.example/"));
    model.activateTabById(first);

    QSignalSpy movedSpy(&model, &TabModel::rowsMoved);
    QSignalSpy activeSpy(&model, &TabModel::activeTabChanged);

    model.moveTab(0, 2);
    QCOMPARE(movedSpy.count(), 1);
    QCOMPARE(role(model, 0, roleId(TabModel::Role::TabId)).toInt(), second);
    QCOMPARE(role(model, 1, roleId(TabModel::Role::TabId)).toInt(), third);
    QCOMPARE(role(model, 2, roleId(TabModel::Role::TabId)).toInt(), first);
    QCOMPARE(model.activeTabId(), first);
    QCOMPARE(model.activeTabIndex(), 2);
    QCOMPARE(activeSpy.count(), 1);
    QVERIFY(role(model, 2, roleId(TabModel::Role::Active)).toBool());

    model.moveTab(2, 1);
    QCOMPARE(role(model, 1, roleId(TabModel::Role::TabId)).toInt(), first);
    QCOMPARE(model.activeTabIndex(), 1);

    movedSpy.clear();
    model.moveTab(1, 1);
    model.moveTab(-1, 0);
    model.moveTab(0, 3);
    model.moveTab(3, 0);
    QCOMPARE(movedSpy.count(), 0);
    QCOMPARE(model.count(), 3);

    const QList<Salama::Tab> stored = persistence.loadTabs();
    QCOMPARE(stored.count(), 3);
    QCOMPARE(stored.at(0).id, second);
    QCOMPARE(stored.at(1).id, first);
    QCOMPARE(stored.at(2).id, third);

    const int fourth = model.newTab(QStringLiteral("https://d.example/"));
    QCOMPARE(persistence.loadTabs().at(3).id, fourth);
}

void tst_tabmodel::closeTabActivatesPrevious()
{
    TabModel model(nullptr);
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    const int b = model.newTab(QStringLiteral("https://b.example/"));
    const int c = model.newTab(QStringLiteral("https://c.example/"));
    QSignalSpy closedSpy(&model, &TabModel::tabClosed);

    model.closeTab(2);
    QCOMPARE(closedSpy.count(), 1);
    QCOMPARE(closedSpy.last().first().toInt(), c);
    QCOMPARE(model.activeTabId(), b);

    model.closeTab(0);
    QCOMPARE(model.activeTabId(), b);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.indexOf(a), -1);

    model.closeTab(7);
    model.closeTab(-1);
    QCOMPARE(model.count(), 1);

    QSignalSpy activeSpy(&model, &TabModel::activeTabChanged);
    model.closeActiveTab();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.activeTabId(), 0);
    QCOMPARE(activeSpy.count(), 1);

    const int d = model.newTab(QStringLiteral("https://d.example/"));
    model.newTab(QStringLiteral("https://e.example/"));
    model.activateTabById(d);
    model.closeTab(0);
    QCOMPARE(model.activeTabIndex(), 0);
}

void tst_tabmodel::closeAllTabs()
{
    TabModel model(nullptr);
    model.newTab(QStringLiteral("https://a.example/"));
    model.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy closedSpy(&model, &TabModel::tabClosed);
    QSignalSpy countSpy(&model, &TabModel::countChanged);

    model.closeAllTabs();
    QCOMPARE(model.count(), 0);
    QCOMPARE(closedSpy.count(), 2);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(model.activeTabId(), 0);
}

void tst_tabmodel::urlUpdatesAndVisits()
{
    TabModel model(nullptr);
    QSignalSpy visitedSpy(&model, &TabModel::visited);
    QSignalSpy dataSpy(&model, &TabModel::activeTabDataChanged);
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    const int b = model.newTab(QStringLiteral("https://b.example/"));
    QCOMPARE(visitedSpy.count(), 0);
    dataSpy.clear();

    model.updateUrl(a, QStringLiteral("https://a.example/"));
    QCOMPARE(visitedSpy.count(), 1);
    model.updateUrl(a, QStringLiteral("https://a.example/"));
    QCOMPARE(visitedSpy.count(), 1);
    QCOMPARE(dataSpy.count(), 0);

    model.updateUrl(a, QStringLiteral("https://a.example/next"));
    QCOMPARE(visitedSpy.count(), 2);
    QCOMPARE(visitedSpy.last().first().toString(), QStringLiteral("https://a.example/next"));
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Url)).toString(),
             QStringLiteral("https://a.example/next"));
    QCOMPARE(dataSpy.count(), 0);

    model.updateUrl(b, QStringLiteral("https://b.example/"));
    QCOMPARE(dataSpy.count(), 1);
    QCOMPARE(model.activeUrl(), QStringLiteral("https://b.example/"));

    model.updateUrl(b, QString());
    model.updateUrl(b, QStringLiteral("tel:112"));
    model.updateUrl(999, QStringLiteral("https://nowhere.example/"));
    QCOMPARE(visitedSpy.count(), 3);
    QCOMPARE(model.activeUrl(), QStringLiteral("https://b.example/"));
}

void tst_tabmodel::titleAndFavicon()
{
    TabModel model(nullptr);
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    const int b = model.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy titleSpy(&model, &TabModel::titleUpdated);
    QSignalSpy faviconSpy(&model, &TabModel::faviconUpdated);
    QSignalSpy dataSpy(&model, &TabModel::activeTabDataChanged);
    QSignalSpy rowSpy(&model, &TabModel::dataChanged);

    model.updateTitle(a, QStringLiteral("A"));
    model.updateTitle(a, QStringLiteral("A"));
    model.updateTitle(999, QStringLiteral("X"));
    QCOMPARE(titleSpy.count(), 1);
    QCOMPARE(titleSpy.last().at(0).toString(), QStringLiteral("https://a.example/"));
    QCOMPARE(titleSpy.last().at(1).toString(), QStringLiteral("A"));
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Title)).toString(), QStringLiteral("A"));
    QCOMPARE(dataSpy.count(), 0);
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>(),
             QVector<int>{roleId(TabModel::Role::Title)});

    model.updateTitle(b, QStringLiteral("B"));
    QCOMPARE(model.activeTitle(), QStringLiteral("B"));
    QCOMPARE(dataSpy.count(), 1);

    model.updateFavicon(a, QStringLiteral("https://a.example/favicon.ico"));
    model.updateFavicon(a, QStringLiteral("https://a.example/favicon.ico"));
    model.updateFavicon(999, QStringLiteral("x"));
    QCOMPARE(faviconSpy.count(), 1);
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Favicon)).toString(),
             QStringLiteral("https://a.example/favicon.ico"));
    model.updateFavicon(b, QStringLiteral("https://b.example/icon.png"));
    QCOMPARE(model.activeFavicon(), QStringLiteral("https://b.example/icon.png"));
    QCOMPARE(dataSpy.count(), 2);
}

void tst_tabmodel::recentThumbnailsFollowTheFront()
{
    QTemporaryDir dir;
    TabModel model(nullptr, dir.path());
    const int first = model.newTab(QStringLiteral("https://first.example/"));
    const int second = model.newTab(QStringLiteral("https://second.example/"));
    const int third = model.newTab(QStringLiteral("https://third.example/"));
    QSignalSpy recent(&model, &TabModel::recentTabsChanged);
    const auto recentThumbnails = [&model]() {
        return model.groupThumbnails(model.defaultGroupId(), 10);
    };

    QCOMPARE(recentThumbnails().count(), 3);
    for (const QString &thumbnail : recentThumbnails()) {
        QVERIFY(thumbnail.isEmpty());
    }

    const QString firstShot = model.thumbnailPath(first);
    QVERIFY(writeFile(firstShot));
    model.updateThumbnail(first, firstShot);
    const QString thirdShot = model.thumbnailPath(third);
    QVERIFY(writeFile(thirdShot));
    model.updateThumbnail(third, thirdShot);
    QVERIFY(recent.count() > 0);

    QCOMPARE(recentThumbnails().first(), thirdShot);

    recent.clear();
    model.activateTabById(first);
    QVERIFY(recent.count() > 0);
    QCOMPARE(recentThumbnails().first(), firstShot);

    // Grid moves don't reorder pictures: position change, not recency.
    model.moveTab(0, 2);
    QCOMPARE(recentThumbnails().first(), firstShot);

    // Never-front tab keeps grid order, not arbitrary: second opened before third, after it
    // here only because third activated later.
    model.activateTabById(second);
    QCOMPARE(recentThumbnails().at(1), firstShot);
    QCOMPARE(recentThumbnails().at(2), thirdShot);

    model.closeTab(model.indexOf(first));
    QCOMPARE(recentThumbnails().count(), 2);
    QVERIFY(!recentThumbnails().contains(firstShot));
}

void tst_tabmodel::recentOrderSurvivesARestart()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    QTemporaryDir shots;
    QString wanted;
    {
        TabModel model(&persistence, shots.path());
        model.newTab(QStringLiteral("https://first.example/"));
        const int second = model.newTab(QStringLiteral("https://second.example/"));
        model.newTab(QStringLiteral("https://third.example/"));
        wanted = model.thumbnailPath(second);
        QVERIFY(writeFile(wanted));
        model.updateThumbnail(second, wanted);
        model.activateTabById(second);
    }
    {
        TabModel model(&persistence, shots.path());
        const QStringList recent = model.groupThumbnails(model.defaultGroupId(), 10);
        QCOMPARE(recent.count(), 3);
        QCOMPARE(recent.first(), wanted);
    }
}

void tst_tabmodel::persistenceRoundTrip()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    int publicId = 0;
    int otherId = 0;
    {
        TabModel model(&persistence);
        publicId = model.newTab(QStringLiteral("https://public.example/"));
        model.newTab(QStringLiteral("https://second.example/"));
        model.activateTabById(publicId);
        otherId = model.newTab(QStringLiteral("https://other.example/"));
        model.updateTitle(publicId, QStringLiteral("Public"));
        model.updateFavicon(publicId, QStringLiteral("https://public.example/favicon.ico"));
        model.updateUrl(otherId, QStringLiteral("https://other.example/moved"));
        model.activateTabById(publicId);
    }
    {
        TabModel model(&persistence);
        QCOMPARE(model.count(), 3);
        QCOMPARE(model.activeTabId(), publicId);
        QCOMPARE(model.activeTitle(), QStringLiteral("Public"));
        QCOMPARE(model.activeFavicon(), QStringLiteral("https://public.example/favicon.ico"));
        QCOMPARE(role(model, 2, roleId(TabModel::Role::Url)).toString(),
                 QStringLiteral("https://other.example/moved"));
        QCOMPARE(role(model, 1, roleId(TabModel::Role::Group)).toInt(), model.defaultGroupId());

        QSignalSpy visitedSpy(&model, &TabModel::visited);
        model.updateUrl(publicId, QStringLiteral("https://public.example/"));
        QCOMPARE(visitedSpy.count(), 0);

        const int fresh = model.newTab(QStringLiteral("https://fresh.example/"));
        QVERIFY(fresh > otherId);
        model.closeTab(model.indexOf(publicId));
        model.closeAllTabs();
    }
    {
        TabModel model(&persistence);
        QCOMPARE(model.count(), 0);
        QCOMPARE(model.activeTabId(), 0);
    }
    {
        TabModel model(&persistence);
        model.newTab(QStringLiteral("https://x.example/"));
        model.newTab(QStringLiteral("https://y.example/"));
        persistence.setActiveTabId(4242);
    }
    {
        TabModel model(&persistence);
        QCOMPARE(model.activeTabIndex(), 0);
    }
}

void tst_tabmodel::thumbnailsAreCapturedPerTab()
{
    QTemporaryDir dir;
    TabModel model(nullptr, dir.path() + QStringLiteral("/previews"));
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    QSignalSpy rowSpy(&model, &TabModel::dataChanged);

    QVERIFY(model.thumbnailPath(4242).isEmpty());

    const QString first = model.thumbnailPath(a);
    QVERIFY(!first.isEmpty());
    QCOMPARE(QFileInfo(first).absolutePath(),
             QDir(dir.path() + QStringLiteral("/previews")).absolutePath());
    // Unique name per capture: new image never masked by cached one.
    QVERIFY(model.thumbnailPath(a) != first);

    QVERIFY(writeFile(first));
    model.updateThumbnail(a, first);
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Thumbnail)).toString(), first);
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>(),
             QVector<int>{roleId(TabModel::Role::Thumbnail)});
    QCOMPARE(model.roleNames().value(roleId(TabModel::Role::Thumbnail)),
             QByteArrayLiteral("thumbnail"));

    const QString second = model.thumbnailPath(a);
    QVERIFY(writeFile(second));
    model.updateThumbnail(a, second);
    QVERIFY(!QFile::exists(first));
    QVERIFY(QFile::exists(second));

    model.updateThumbnail(a, second);
    QCOMPARE(rowSpy.count(), 2);

    const QString outside = dir.path() + QStringLiteral("/keep.png");
    QVERIFY(writeFile(outside));
    model.updateThumbnail(a, outside);
    model.updateThumbnail(a, model.thumbnailPath(a));
    QVERIFY(QFile::exists(outside));
}

// Picture encoded and written on worker, never GUI thread: half-screen encode in grab
// callback stuttered grid-open drag (#27).
void tst_tabmodel::thumbnailsAreWrittenOffTheGuiThread()
{
    QTemporaryDir dir;
    TabModel model(nullptr, dir.path() + QStringLiteral("/previews"));
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    QThread *writtenOn = nullptr;
    connect(
        model.thumbnailWriter(), &Salama::ThumbnailWriter::written, this,
        [&writtenOn]() { writtenOn = QThread::currentThread(); }, Qt::DirectConnection);

    QImage picture(270, 600, QImage::Format_ARGB32_Premultiplied);
    picture.fill(Qt::darkCyan);
    QVERIFY(model.storeThumbnail(a, QVariant::fromValue(picture)));
    // No signal before file complete; never within call.
    QVERIFY(role(model, 0, roleId(TabModel::Role::Thumbnail)).toString().isEmpty());
    QTRY_VERIFY(!role(model, 0, roleId(TabModel::Role::Thumbnail)).toString().isEmpty());
    QVERIFY(writtenOn != nullptr);
    QVERIFY(writtenOn != QThread::currentThread());

    const QString path = role(model, 0, roleId(TabModel::Role::Thumbnail)).toString();
    QCOMPARE(QFileInfo(path).absolutePath(),
             QDir(dir.path() + QStringLiteral("/previews")).absolutePath());
    QImageReader reader(path);
    QCOMPARE(reader.format(), QByteArrayLiteral("png"));
    QCOMPARE(reader.size(), picture.size());

    QVERIFY(!model.storeThumbnail(a, QVariant::fromValue(QImage())));
    QVERIFY(!model.storeThumbnail(a, QVariant()));
    QVERIFY(!model.storeThumbnail(4242, QVariant::fromValue(picture)));
    TabModel noPreviews(nullptr, QString());
    QVERIFY(!noPreviews.storeThumbnail(noPreviews.newTab(QStringLiteral("https://b.example/")),
                                       QVariant::fromValue(picture)));
}

// Late write superseded by newer picture, or after tab closed / back on start page: no
// file, no change.
void tst_tabmodel::staleThumbnailWritesAreDiscarded()
{
    QTemporaryDir dir;
    const QString previews = dir.path() + QStringLiteral("/previews");
    TabModel model(nullptr, previews);
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    const int b = model.newTab(QStringLiteral("https://b.example/"));
    QImage picture(32, 64, QImage::Format_ARGB32_Premultiplied);
    picture.fill(Qt::darkCyan);
    const auto files = [&previews]() { return QDir(previews).entryList(QDir::Files); };

    QVERIFY(model.storeThumbnail(a, QVariant::fromValue(picture)));
    QVERIFY(model.storeThumbnail(a, QVariant::fromValue(picture)));
    QVERIFY(model.storeThumbnail(b, QVariant::fromValue(picture)));
    model.closeTab(model.indexOf(b));
    model.thumbnailWriter()->waitForDone();
    QCoreApplication::processEvents();
    QCOMPARE(files().count(), 1);
    const QString kept =
        role(model, model.indexOf(a), roleId(TabModel::Role::Thumbnail)).toString();
    QCOMPARE(QFileInfo(kept).fileName(), files().first());

    const int c = model.newTab(QStringLiteral("https://c.example/"));
    QVERIFY(model.storeThumbnail(c, QVariant::fromValue(picture)));
    model.showStartPage(c);
    model.thumbnailWriter()->waitForDone();
    QCoreApplication::processEvents();
    QVERIFY(role(model, model.indexOf(c), roleId(TabModel::Role::Thumbnail)).toString().isEmpty());
    QCOMPARE(files().count(), 1);
}

void tst_tabmodel::thumbnailsFollowTabLifetime()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    const QString previews = dir.path() + QStringLiteral("/previews");
    QString kept;
    int keptId = 0;
    {
        TabModel model(&persistence, previews);
        const int a = model.newTab(QStringLiteral("https://a.example/"));
        const int b = model.newTab(QStringLiteral("https://b.example/"));
        const QString shotA = model.thumbnailPath(a);
        const QString shotB = model.thumbnailPath(b);
        QVERIFY(writeFile(shotA));
        QVERIFY(writeFile(shotB));
        model.updateThumbnail(a, shotA);
        model.updateThumbnail(b, shotB);

        model.closeTab(model.indexOf(a));
        QVERIFY(!QFile::exists(shotA));
        QVERIFY(QFile::exists(shotB));
        kept = shotB;
        keptId = b;
    }
    {
        TabModel model(&persistence, previews);
        QCOMPARE(model.count(), 1);
        QCOMPARE(role(model, 0, roleId(TabModel::Role::Thumbnail)).toString(), kept);

        model.closeAllTabs();
        QVERIFY(!QFile::exists(kept));
    }
    {
        TabModel model(&persistence, previews);
        const int c = model.newTab(QStringLiteral("https://c.example/"));
        const QString shot = model.thumbnailPath(c);
        QVERIFY(writeFile(shot));
        model.updateThumbnail(c, shot);
        QVERIFY(QFile::remove(shot));
        Q_UNUSED(keptId)
    }
    TabModel model(&persistence, previews);
    QCOMPARE(model.count(), 1);
    QVERIFY(role(model, 0, roleId(TabModel::Role::Thumbnail)).toString().isEmpty());
}

void tst_tabmodel::thumbnailsAreOptional()
{
    TabModel model(nullptr);
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    QVERIFY(model.thumbnailPath(a).isEmpty());
    model.updateThumbnail(a, QStringLiteral("/tmp/nowhere.png"));
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Thumbnail)).toString(),
             QStringLiteral("/tmp/nowhere.png"));
    model.closeAllTabs();
}

void tst_tabmodel::thereIsAlwaysAGroup()
{
    TabModel model(nullptr);
    QCOMPARE(model.groups().count(), 1);
    const int groupId = model.defaultGroupId();
    QVERIFY(groupId > 0);
    QCOMPARE(model.groups().first().id, groupId);
    QCOMPARE(model.currentGroupId(), groupId);
    QCOMPARE(model.currentGroupIndex(), 0);
    QCOMPARE(model.groupModel()->count(), 1);
    QCOMPARE(model.groupModel()->rowCount(model.groupModel()->index(0, 0)), 0);
    QCOMPARE(model.groupModel()->groupIdAt(0), groupId);
    QCOMPARE(model.groupModel()->groupIdAt(1), 0);
    QVERIFY(role(*model.groupModel(), 0, roleId(TabGroupModel::Role::Name)).toString().isEmpty());
    QCOMPARE(role(*model.groupModel(), 0, roleId(TabGroupModel::Role::TabCount)).toInt(), 0);
    QVERIFY(role(*model.groupModel(), 0, roleId(TabGroupModel::Role::Current)).toBool());
    QVERIFY(role(*model.groupModel(), 0, roleId(TabGroupModel::Role::Default)).toBool());
    QVERIFY(!role(*model.groupModel(), 1, roleId(TabGroupModel::Role::Current)).isValid());
    QCOMPARE(model.groupModel()->roleNames().value(roleId(TabGroupModel::Role::TabCount)),
             QByteArrayLiteral("tabCount"));
    QCOMPARE(model.groupModel()->roleNames().value(roleId(TabGroupModel::Role::Default)),
             QByteArrayLiteral("defaultGroup"));
    QCOMPARE(model.roleNames().value(roleId(TabModel::Role::Group)), QByteArrayLiteral("groupId"));

    QVERIFY(!model.removeGroup(groupId));
    QVERIFY(!model.removeGroup(4242));
    model.setCurrentGroupId(4242);
    QCOMPARE(model.currentGroupId(), groupId);
    model.groupModel()->activate(7);
    QCOMPARE(model.currentGroupId(), groupId);
    QVERIFY(!model.moveTabToGroup(1, groupId));
    model.renameGroup(4242, QStringLiteral("Nowhere"));
    model.renameGroup(groupId, QStringLiteral("Home"));
    QVERIFY(model.groups().first().name.isEmpty());

    const int a = model.newTab(QStringLiteral("https://a.example/"));
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Group)).toInt(), groupId);
    QCOMPARE(model.groupTabs()->count(), 1);
    QCOMPARE(role(*model.groupModel(), 0, roleId(TabGroupModel::Role::TabCount)).toInt(), 1);
    QCOMPARE(model.groupTabs()->rowCount(model.groupTabs()->index(0, 0)), 0);
    QCOMPARE(model.groupTabs()->tabIdAt(0), a);
    QCOMPARE(model.groupTabs()->rowOf(a), 0);
    QCOMPARE(model.groupTabs()->rowOf(4242), -1);
    QCOMPARE(role(*model.groupTabs(), 0, roleId(TabModel::Role::Url)).toString(),
             QStringLiteral("https://a.example/"));
    QVERIFY(!role(*model.groupTabs(), 1, roleId(TabModel::Role::Url)).isValid());
    QCOMPARE(model.groupTabs()->roleNames(), model.roleNames());
}

void tst_tabmodel::groupsHoldTheirOwnTabs()
{
    TabModel model(nullptr);
    const int home = model.defaultGroupId();
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    const int b = model.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy groupSpy(&model, &TabModel::currentGroupChanged);
    QSignalSpy groupsSpy(&model, &TabModel::groupsChanged);
    QSignalSpy insertSpy(model.groupModel(), &TabGroupModel::rowsInserted);
    QSignalSpy resetSpy(model.groupTabs(), &GroupTabModel::modelReset);

    const int work = model.addGroup(QStringLiteral("  Work "));
    QVERIFY(work > home);
    QCOMPARE(model.groups().count(), 2);
    QCOMPARE(model.currentGroupId(), work);
    QCOMPARE(model.currentGroupIndex(), 1);
    QCOMPARE(model.groups().last().id, work);
    QCOMPARE(groupSpy.count(), 1);
    QCOMPARE(groupsSpy.count(), 1);
    QCOMPARE(insertSpy.count(), 1);
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.groupTabs()->count(), 0);
    QCOMPARE(model.activeTabId(), b);
    QCOMPARE(role(*model.groupModel(), 1, roleId(TabGroupModel::Role::Name)).toString(),
             QStringLiteral("Work"));
    QVERIFY(role(*model.groupModel(), 1, roleId(TabGroupModel::Role::Current)).toBool());
    QVERIFY(!role(*model.groupModel(), 1, roleId(TabGroupModel::Role::Default)).toBool());
    QVERIFY(!role(*model.groupModel(), 0, roleId(TabGroupModel::Role::Current)).toBool());

    const int c = model.newTab(QStringLiteral("https://c.example/"));
    QCOMPARE(role(model, 2, roleId(TabModel::Role::Group)).toInt(), work);
    QCOMPARE(groupTabIds(*model.groupTabs()), QList<int>{c});
    QCOMPARE(role(*model.groupModel(), 1, roleId(TabGroupModel::Role::TabCount)).toInt(), 1);
    QCOMPARE(role(*model.groupModel(), 0, roleId(TabGroupModel::Role::TabCount)).toInt(), 2);
    QCOMPARE(model.count(), 3);

    model.activateTabById(a);
    model.activateTabById(c);
    model.groupModel()->activate(0);
    QCOMPARE(model.currentGroupId(), home);
    QCOMPARE(model.activeTabId(), a);
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{a, b}));
    QCOMPARE(groupSpy.count(), 4);

    model.activateTabById(c);
    QCOMPARE(model.currentGroupId(), work);
    QCOMPARE(groupTabIds(*model.groupTabs()), QList<int>{c});
    model.setCurrentGroupId(work);
    QCOMPARE(groupSpy.count(), 5);

    model.renameGroup(work, QStringLiteral("Play"));
    QCOMPARE(model.groups().at(1).name, QStringLiteral("Play"));
    model.renameGroup(work, QStringLiteral("Play"));
    QCOMPARE(groupsSpy.count(), 2);
}

void tst_tabmodel::movingTabsInsideAGroup()
{
    TabModel model(nullptr);
    const int a1 = model.newTab(QStringLiteral("https://a1.example/"));
    const int other = model.addGroup(QStringLiteral("Other"));
    const int b1 = model.newTab(QStringLiteral("https://b1.example/"));
    model.activateTabById(a1);
    const int a2 = model.newTab(QStringLiteral("https://a2.example/"));
    model.activateTabById(b1);
    const int b2 = model.newTab(QStringLiteral("https://b2.example/"));
    model.activateTabById(a2);
    const int a3 = model.newTab(QStringLiteral("https://a3.example/"));
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{a1, a2, a3}));
    QSignalSpy moveSpy(model.groupTabs(), &GroupTabModel::rowsMoved);

    model.groupTabs()->moveTab(2, 0);
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{a3, a1, a2}));
    QCOMPARE(moveSpy.count(), 1);
    QCOMPARE(model.indexOf(a3), 0);
    QCOMPARE(model.indexOf(b1), 2);
    QCOMPARE(model.indexOf(b2), 4);
    QCOMPARE(model.activeTabId(), a3);

    // Back to group end = right after a2 in model: other group's tabs not in order; a2 where
    // finger let go.
    model.groupTabs()->moveTab(0, 2);
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{a1, a2, a3}));
    QCOMPARE(moveSpy.count(), 2);
    QCOMPARE(model.indexOf(a3), 3);
    QCOMPARE(model.indexOf(b2), 4);

    model.groupTabs()->moveTab(1, 1);
    model.groupTabs()->moveTab(-1, 1);
    model.groupTabs()->moveTab(1, 3);
    QCOMPARE(moveSpy.count(), 2);

    model.moveTab(model.indexOf(b2), model.indexOf(b1));
    QCOMPARE(moveSpy.count(), 2);
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{a1, a2, a3}));
    model.groupModel()->activate(model.groupIndexOf(other));
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{b2, b1}));
}

void tst_tabmodel::closingStaysInTheGroup()
{
    TabModel model(nullptr);
    const int home = model.defaultGroupId();
    const int a1 = model.newTab(QStringLiteral("https://a1.example/"));
    const int work = model.addGroup(QStringLiteral("Work"));
    const int b1 = model.newTab(QStringLiteral("https://b1.example/"));
    model.activateTabById(a1);
    const int a2 = model.newTab(QStringLiteral("https://a2.example/"));
    const int a3 = model.newTab(QStringLiteral("https://a3.example/"));

    model.activateTabById(a2);
    model.closeTabById(a2);
    QCOMPARE(model.activeTabId(), a1);
    QCOMPARE(model.currentGroupId(), home);
    model.activateTabById(a1);
    model.closeTab(model.indexOf(a1));
    QCOMPARE(model.activeTabId(), a3);
    QCOMPARE(model.currentGroupId(), home);
    QCOMPARE(groupTabIds(*model.groupTabs()), QList<int>{a3});

    model.closeTabById(a3);
    QCOMPARE(model.activeTabId(), b1);
    QCOMPARE(model.currentGroupId(), work);
    QCOMPARE(model.groups().count(), 2);
    QCOMPARE(model.tabCountInGroup(home), 0);

    model.setCurrentGroupId(home);
    QCOMPARE(model.activeTabId(), b1);
    QCOMPARE(model.groupTabs()->count(), 0);
    const int a4 = model.newTab(QStringLiteral("https://a4.example/"));
    QCOMPARE(role(model, model.indexOf(a4), roleId(TabModel::Role::Group)).toInt(), home);
    QCOMPARE(model.activeTabId(), a4);

    model.closeTabById(b1);
    QCOMPARE(model.activeTabId(), a4);
    QCOMPARE(model.currentGroupId(), home);
    model.closeTabById(4242);
    QCOMPARE(model.count(), 1);

    model.closeAllTabs();
    QCOMPARE(model.groups().count(), 2);
    QCOMPARE(model.groupTabs()->count(), 0);
    QCOMPARE(role(*model.groupModel(), 1, roleId(TabGroupModel::Role::TabCount)).toInt(), 0);
}

void tst_tabmodel::removingAGroupClosesItsTabs()
{
    TabModel model(nullptr);
    const int home = model.defaultGroupId();
    const int a1 = model.newTab(QStringLiteral("https://a1.example/"));
    const int work = model.addGroup(QStringLiteral("Work"));
    const int b1 = model.newTab(QStringLiteral("https://b1.example/"));
    const int b2 = model.newTab(QStringLiteral("https://b2.example/"));
    const int play = model.addGroup(QStringLiteral("Play"));
    const int c1 = model.newTab(QStringLiteral("https://c1.example/"));
    QSignalSpy closedSpy(&model, &TabModel::tabClosed);
    QSignalSpy removedSpy(model.groupModel(), &TabGroupModel::rowsRemoved);

    model.activateTabById(b2);
    QCOMPARE(model.currentGroupId(), work);
    QVERIFY(model.removeGroup(work));
    QCOMPARE(model.groups().count(), 2);
    QCOMPARE(model.groupIndexOf(work), -1);
    QCOMPARE(closedSpy.count(), 2);
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(model.indexOf(b1), -1);
    QCOMPARE(model.indexOf(b2), -1);
    QCOMPARE(model.currentGroupId(), home);
    QCOMPARE(model.activeTabId(), a1);
    QCOMPARE(model.currentGroupIndex(), 0);
    QCOMPARE(model.groupIndexOf(play), 1);

    QVERIFY(!model.removeGroup(home));
    model.activateTabById(c1);
    QCOMPARE(model.currentGroupId(), play);
    QVERIFY(model.removeGroup(play));
    QCOMPARE(model.groups().count(), 1);
    QCOMPARE(model.currentGroupId(), home);
    QCOMPARE(model.activeTabId(), a1);
    QCOMPARE(model.count(), 1);
}

void tst_tabmodel::movingATabToAnotherGroup()
{
    TabModel model(nullptr);
    const int home = model.defaultGroupId();
    const int a1 = model.newTab(QStringLiteral("https://a1.example/"));
    const int a2 = model.newTab(QStringLiteral("https://a2.example/"));
    const int work = model.addGroup(QStringLiteral("Work"));
    model.setCurrentGroupId(home);
    QCOMPARE(model.activeTabId(), a2);
    QSignalSpy rowSpy(&model, &TabModel::dataChanged);
    QSignalSpy moveSpy(&model, &TabModel::rowsMoved);
    QSignalSpy removeSpy(&model, &TabModel::rowsRemoved);

    QVERIFY(model.moveTabToGroup(a2, work));
    QCOMPARE(model.indexOf(a2), 1);
    QCOMPARE(role(model, 1, roleId(TabModel::Role::Group)).toInt(), work);
    QCOMPARE(moveSpy.count(), 0);
    QCOMPARE(removeSpy.count(), 0);
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>(),
             QVector<int>{roleId(TabModel::Role::Group)});
    QCOMPARE(model.currentGroupId(), work);
    QCOMPARE(model.activeTabId(), a2);
    QCOMPARE(groupTabIds(*model.groupTabs()), QList<int>{a2});
    QCOMPARE(model.tabCountInGroup(home), 1);

    QVERIFY(model.moveTabToGroup(a1, work));
    QCOMPARE(model.currentGroupId(), work);
    QCOMPARE(model.activeTabId(), a2);
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{a2, a1}));
    QVERIFY(model.moveTabToGroup(a1, work));
    QCOMPARE(model.tabCountInGroup(home), 0);
    QVERIFY(!model.moveTabToGroup(a1, 4242));
    QVERIFY(!model.moveTabToGroup(4242, home));
    QVERIFY(model.groupModel()->moveTab(a1, home));
    QCOMPARE(groupTabIds(*model.groupTabs()), QList<int>{a2});
}

void tst_tabmodel::groupsSurviveARestart()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    int home = 0;
    int work = 0;
    int a1 = 0;
    int b1 = 0;
    {
        TabModel model(&persistence);
        home = model.defaultGroupId();
        a1 = model.newTab(QStringLiteral("https://a1.example/"));
        work = model.addGroup(QStringLiteral("Work"));
        b1 = model.newTab(QStringLiteral("https://b1.example/"));
        model.renameGroup(work, QStringLiteral("Office"));
        model.activateTabById(b1);
    }
    {
        TabModel model(&persistence);
        QCOMPARE(model.groups().count(), 2);
        QCOMPARE(model.groups().at(0).id, home);
        QCOMPARE(model.groups().at(1).name, QStringLiteral("Office"));
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.activeTabId(), b1);
        QCOMPARE(model.currentGroupId(), work);
        QCOMPARE(groupTabIds(*model.groupTabs()), QList<int>{b1});
        QCOMPARE(role(model, 0, roleId(TabModel::Role::Group)).toInt(), home);
        const int play = model.addGroup(QStringLiteral("Play"));
        QVERIFY(play > work);
        QVERIFY(model.removeGroup(work));
        QCOMPARE(model.activeTabId(), a1);
        QCOMPARE(model.currentGroupId(), home);
        QVERIFY(model.removeGroup(play));
    }
    {
        TabModel model(&persistence);
        QCOMPARE(model.groups().count(), 1);
        QCOMPARE(model.currentGroupId(), home);
        QCOMPARE(model.activeTabId(), a1);
        QVERIFY(model.addGroup(QStringLiteral("Again")) > home);
    }
    {
        // Tab with group lacking row (all tabs from pre-schema-4 DB): group created, unnamed.
        Tab stray;
        stray.id = 77;
        stray.url = QStringLiteral("https://stray.example/");
        stray.groupId = 40;
        persistence.insertTab(stray);
        persistence.setCurrentGroupId(4242);
        persistence.setActiveTabId(77);
        TabModel model(&persistence);
        QCOMPARE(model.groups().count(), 3);
        const TabGroup strayGroup = model.groups().at(2);
        QCOMPARE(strayGroup.id, 40);
        QVERIFY(strayGroup.name.isEmpty());
        QCOMPARE(model.groups().first().id, home);
        QCOMPARE(model.currentGroupId(), 40);
        QVERIFY(model.addGroup(QString()) > 40);
    }
}

void tst_tabmodel::ungroupingKeepsTheTabs()
{
    TabModel model(nullptr);
    const int home = model.defaultGroupId();
    const int a1 = model.newTab(QStringLiteral("https://a1.example/"));
    const int work = model.addGroup(QStringLiteral("Work"));
    const int b1 = model.newTab(QStringLiteral("https://b1.example/"));
    const int b2 = model.newTab(QStringLiteral("https://b2.example/"));
    const int play = model.addGroup(QStringLiteral("Play"));
    const int c1 = model.newTab(QStringLiteral("https://c1.example/"));
    QSignalSpy closedSpy(&model, &TabModel::tabClosed);
    QSignalSpy moveSpy(&model, &TabModel::rowsMoved);
    QSignalSpy removedSpy(model.groupModel(), &TabGroupModel::rowsRemoved);
    QSignalSpy groupsSpy(&model, &TabModel::groupsChanged);

    QVERIFY(!model.ungroup(home));
    QVERIFY(!model.groupModel()->ungroup(4242));
    QCOMPARE(model.groups().count(), 3);

    model.activateTabById(b2);
    model.activateTabById(c1);
    QCOMPARE(model.currentGroupId(), play);
    QVERIFY(model.groupModel()->ungroup(work));
    QCOMPARE(model.groups().count(), 2);
    QCOMPARE(model.groupIndexOf(work), -1);
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(groupsSpy.count(), 1);
    QCOMPARE(closedSpy.count(), 0);
    QCOMPARE(moveSpy.count(), 0);
    QCOMPARE(model.count(), 4);
    QCOMPARE(model.indexOf(b1), 1);
    QCOMPARE(role(model, model.indexOf(b1), roleId(TabModel::Role::Group)).toInt(), home);
    QCOMPARE(role(model, model.indexOf(b2), roleId(TabModel::Role::Group)).toInt(), home);
    QCOMPARE(model.tabCountInGroup(home), 3);
    QCOMPARE(role(*model.groupModel(), 0, roleId(TabGroupModel::Role::TabCount)).toInt(), 3);
    QCOMPARE(model.currentGroupId(), play);
    QCOMPARE(model.currentGroupIndex(), 1);
    QCOMPARE(model.activeTabId(), c1);
    QCOMPARE(groupTabIds(*model.groupTabs()), QList<int>{c1});

    QVERIFY(model.ungroup(play));
    QCOMPARE(model.groups().count(), 1);
    QCOMPARE(model.currentGroupId(), home);
    QCOMPARE(model.activeTabId(), c1);
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{a1, b1, b2, c1}));
    QCOMPARE(closedSpy.count(), 0);
    QCOMPARE(model.count(), 4);

    const int mail = model.addGroup(QStringLiteral("Mail"));
    const int d1 = model.newTab(QStringLiteral("https://d1.example/"));
    model.activateTabById(a1);
    QCOMPARE(model.currentGroupId(), home);
    QVERIFY(model.ungroup(mail));
    QCOMPARE(groupTabIds(*model.groupTabs()), (QList<int>{a1, b1, b2, c1, d1}));
    model.activateTabById(b2);
    const int side = model.addGroup(QStringLiteral("Side"));
    const int s1 = model.newTab(QStringLiteral("https://s1.example/"));
    const int empty = model.addGroup(QString());
    QCOMPARE(model.currentGroupId(), empty);
    QCOMPARE(model.activeTabId(), s1);
    QVERIFY(model.ungroup(empty));
    QCOMPARE(model.currentGroupId(), home);
    QCOMPARE(model.activeTabId(), b2);
    QCOMPARE(model.groups().count(), 2);
    QCOMPARE(model.groupIndexOf(side), 1);
}

void tst_tabmodel::movingGroups()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    int home = 0;
    int work = 0;
    int play = 0;
    int mail = 0;
    {
        TabModel model(&persistence);
        home = model.defaultGroupId();
        work = model.addGroup(QStringLiteral("Work"));
        play = model.addGroup(QStringLiteral("Play"));
        mail = model.addGroup(QStringLiteral("Mail"));
        model.setCurrentGroupId(work);
        QSignalSpy moveSpy(model.groupModel(), &TabGroupModel::rowsMoved);
        QSignalSpy groupsSpy(&model, &TabModel::groupsChanged);
        QSignalSpy currentSpy(&model, &TabModel::currentGroupChanged);

        QVERIFY(model.groupModel()->moveGroup(1, 3));
        QCOMPARE(model.groups().at(1).id, play);
        QCOMPARE(model.groups().at(2).id, mail);
        QCOMPARE(model.groups().at(3).id, work);
        QCOMPARE(moveSpy.count(), 1);
        QCOMPARE(moveSpy.last().at(1).toInt(), 1);
        QCOMPARE(moveSpy.last().at(4).toInt(), 4);
        QCOMPARE(groupsSpy.count(), 1);
        QCOMPARE(currentSpy.count(), 1);
        QCOMPARE(model.currentGroupId(), work);
        QCOMPARE(model.currentGroupIndex(), 3);
        QCOMPARE(model.groupModel()->groupIdAt(3), work);

        QVERIFY(model.moveGroup(2, 1));
        QCOMPARE(model.groups().at(1).id, mail);
        QCOMPARE(model.groups().at(2).id, play);
        QCOMPARE(moveSpy.count(), 2);
        QCOMPARE(moveSpy.last().at(4).toInt(), 1);
        QCOMPARE(currentSpy.count(), 1);

        QVERIFY(!model.moveGroup(0, 2));
        QVERIFY(!model.moveGroup(2, 0));
        QVERIFY(!model.moveGroup(1, 4));
        QVERIFY(!model.moveGroup(-1, 1));
        QVERIFY(!model.moveGroup(2, 2));
        QCOMPARE(moveSpy.count(), 2);
        QCOMPARE(groupsSpy.count(), 2);
        QCOMPARE(model.groups().first().id, home);
    }
    {
        TabModel model(&persistence);
        QCOMPARE(model.groups().count(), 4);
        QCOMPARE(model.groups().at(0).id, home);
        QCOMPARE(model.groups().at(1).id, mail);
        QCOMPARE(model.groups().at(2).id, play);
        QCOMPARE(model.groups().at(3).id, work);
        const int news = model.addGroup(QStringLiteral("News"));
        QCOMPARE(model.groups().last().id, news);
    }
}

void tst_tabmodel::groupPreviews()
{
    TabModel model(nullptr);
    const int home = model.defaultGroupId();
    QCOMPARE(model.groupModel()->roleNames().value(roleId(TabGroupModel::Role::Previews)),
             QByteArrayLiteral("previews"));
    const auto previews = [&model](int row) {
        return role(*model.groupModel(), row, roleId(TabGroupModel::Role::Previews)).toStringList();
    };
    QVERIFY(previews(0).isEmpty());
    QVERIFY(model.groupThumbnails(4242, 4).isEmpty());

    QList<int> ids;
    for (int i = 0; i < 5; ++i) {
        ids.append(model.newTab(QStringLiteral("https://t%1.example/").arg(i)));
        model.updateThumbnail(ids.last(), QStringLiteral("/previews/t%1.png").arg(i));
    }
    const int limit = TabGroupModel::PreviewLimit;
    QCOMPARE(limit, 4);
    QCOMPARE(previews(0),
             (QStringList{QStringLiteral("/previews/t4.png"), QStringLiteral("/previews/t3.png"),
                          QStringLiteral("/previews/t2.png"), QStringLiteral("/previews/t1.png")}));
    const int bare = model.newTab(QStringLiteral("https://bare.example/"));
    QCOMPARE(previews(0).first(), QString());
    QCOMPARE(model.groupThumbnails(home, 2),
             (QStringList{QString(), QStringLiteral("/previews/t4.png")}));

    QSignalSpy changeSpy(model.groupModel(), &TabGroupModel::dataChanged);
    const auto told = [&changeSpy](int row) {
        return std::any_of(changeSpy.cbegin(), changeSpy.cend(),
                           [row](const QList<QVariant> &change) {
                               return change.at(0).toModelIndex().row() <= row &&
                                      change.at(1).toModelIndex().row() >= row &&
                                      change.at(2).value<QVector<int>>().contains(
                                          roleId(TabGroupModel::Role::Previews));
                           });
    };
    const int work = model.addGroup(QStringLiteral("Work"));
    QVERIFY(previews(1).isEmpty());
    QVERIFY(model.moveTabToGroup(ids.at(0), work));
    QVERIFY(told(0));
    QVERIFY(told(1));
    QCOMPARE(previews(1), QStringList{QStringLiteral("/previews/t0.png")});
    QCOMPARE(previews(0).count(), limit);
    QVERIFY(!previews(0).contains(QStringLiteral("/previews/t0.png")));

    changeSpy.clear();
    model.activateTabById(ids.at(1));
    QVERIFY(told(0));
    QCOMPARE(previews(0).first(), QStringLiteral("/previews/t1.png"));
    changeSpy.clear();
    model.updateThumbnail(bare, QStringLiteral("/previews/bare.png"));
    QVERIFY(told(0));
    QVERIFY(previews(0).contains(QStringLiteral("/previews/bare.png")));

    changeSpy.clear();
    model.activateTabById(ids.at(0));
    QVERIFY(model.ungroup(work));
    QVERIFY(told(0));
    QCOMPARE(previews(0).first(), QStringLiteral("/previews/t0.png"));
}

void tst_tabmodel::searchSpansTheGroups()
{
    TabModel model(nullptr);
    TabSearchModel search(&model);
    QCOMPARE(search.count(), 0);
    QCOMPARE(search.rowCount(search.index(0, 0)), 0);
    QVERIFY(search.searchTerm().isEmpty());
    QSignalSpy countSpy(&search, &TabSearchModel::countChanged);
    QSignalSpy termSpy(&search, &TabSearchModel::searchTermChanged);

    const int a1 = model.newTab(QStringLiteral("https://news.example/"));
    model.updateTitle(a1, QStringLiteral("Morning news"));
    const int work = model.addGroup(QStringLiteral("Work"));
    const int b1 = model.newTab(QStringLiteral("https://mail.example/"));
    model.updateTitle(b1, QStringLiteral("Inbox"));
    model.activateTabById(a1);
    const int a2 = model.newTab(QStringLiteral("https://weather.example/"));
    model.updateTitle(a2, QStringLiteral("Weather news"));

    QCOMPARE(search.count(), 3);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), a1);
    QCOMPARE(role(search, 1, roleId(TabSearchModel::Role::TabId)).toInt(), a2);
    QCOMPARE(role(search, 2, roleId(TabSearchModel::Role::TabId)).toInt(), b1);
    QVERIFY(role(search, 0, roleId(TabSearchModel::Role::GroupStart)).toBool());
    QVERIFY(!role(search, 1, roleId(TabSearchModel::Role::GroupStart)).toBool());
    QVERIFY(role(search, 2, roleId(TabSearchModel::Role::GroupStart)).toBool());
    QCOMPARE(role(search, 2, roleId(TabSearchModel::Role::GroupId)).toInt(), work);
    QCOMPARE(role(search, 2, roleId(TabSearchModel::Role::GroupName)).toString(),
             QStringLiteral("Work"));
    QVERIFY(role(search, 1, roleId(TabSearchModel::Role::GroupName)).toString().isEmpty());
    QCOMPARE(role(search, 1, roleId(TabSearchModel::Role::GroupTabCount)).toInt(), 2);
    QCOMPARE(role(search, 2, roleId(TabSearchModel::Role::Url)).toString(),
             QStringLiteral("https://mail.example/"));
    QCOMPARE(role(search, 2, roleId(TabSearchModel::Role::Title)).toString(),
             QStringLiteral("Inbox"));
    QVERIFY(role(search, 2, roleId(TabSearchModel::Role::Favicon)).toString().isEmpty());
    QVERIFY(!role(search, 3, roleId(TabSearchModel::Role::TabId)).isValid());
    QCOMPARE(search.roleNames().value(roleId(TabSearchModel::Role::GroupStart)),
             QByteArrayLiteral("groupStart"));

    search.setSearchTerm(QStringLiteral("  NEWS "));
    QCOMPARE(search.searchTerm(), QStringLiteral("NEWS"));
    QCOMPARE(termSpy.count(), 1);
    QCOMPARE(search.count(), 2);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), a1);
    QCOMPARE(role(search, 1, roleId(TabSearchModel::Role::TabId)).toInt(), a2);
    QVERIFY(!role(search, 1, roleId(TabSearchModel::Role::GroupStart)).toBool());
    search.setSearchTerm(QStringLiteral("NEWS"));
    QCOMPARE(termSpy.count(), 1);
    search.setSearchTerm(QStringLiteral("mail.ex"));
    QCOMPARE(search.count(), 1);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), b1);
    QVERIFY(role(search, 0, roleId(TabSearchModel::Role::GroupStart)).toBool());
    search.setSearchTerm(QStringLiteral("nothing"));
    QCOMPARE(search.count(), 0);

    search.setSearchTerm(QString());
    QCOMPARE(search.count(), 3);
    model.renameGroup(work, QStringLiteral("Office"));
    QCOMPARE(role(search, 2, roleId(TabSearchModel::Role::GroupName)).toString(),
             QStringLiteral("Office"));
    model.closeTabById(b1);
    QCOMPARE(search.count(), 2);
    model.updateTitle(a1, QStringLiteral("Evening news"));
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::Title)).toString(),
             QStringLiteral("Evening news"));
    QVERIFY(countSpy.count() >= 3);
}

void tst_tabmodel::searchRefinesWithoutResetting()
{
    TabModel model(nullptr);
    TabSearchModel search(&model);
    const int a = model.newTab(QStringLiteral("https://apple.example/"));
    const int b = model.newTab(QStringLiteral("https://banana.example/"));
    const int work = model.addGroup(QStringLiteral("Work"));
    const int c = model.newTab(QStringLiteral("https://apricot.example/"));
    const int d = model.newTab(QStringLiteral("https://cherry.example/"));
    Q_UNUSED(work)
    QCOMPARE(search.count(), 4);
    QSignalSpy resetSpy(&search, &TabSearchModel::modelReset);
    QSignalSpy removeSpy(&search, &TabSearchModel::rowsRemoved);
    QSignalSpy insertSpy(&search, &TabSearchModel::rowsInserted);
    QSignalSpy changeSpy(&search, &TabSearchModel::dataChanged);

    // Typing removes rows one by one, never resets: page under keyboard keeps place and focus.
    search.setSearchTerm(QStringLiteral("ap"));
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(removeSpy.count(), 2);
    QCOMPARE(insertSpy.count(), 0);
    QCOMPARE(search.count(), 2);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), a);
    QCOMPARE(role(search, 1, roleId(TabSearchModel::Role::TabId)).toInt(), c);
    QVERIFY(role(search, 1, roleId(TabSearchModel::Role::GroupStart)).toBool());

    search.setSearchTerm(QStringLiteral("apr"));
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(removeSpy.count(), 3);
    QCOMPARE(search.count(), 1);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), c);
    QVERIFY(role(search, 0, roleId(TabSearchModel::Role::GroupStart)).toBool());

    search.setSearchTerm(QStringLiteral("a"));
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(insertSpy.count(), 3);
    QCOMPARE(search.count(), 4);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), a);
    QCOMPARE(role(search, 1, roleId(TabSearchModel::Role::TabId)).toInt(), b);
    QCOMPARE(role(search, 2, roleId(TabSearchModel::Role::TabId)).toInt(), c);
    QCOMPARE(role(search, 3, roleId(TabSearchModel::Role::TabId)).toInt(), d);
    QVERIFY(!role(search, 1, roleId(TabSearchModel::Role::GroupStart)).toBool());
    QVERIFY(!role(search, 3, roleId(TabSearchModel::Role::GroupStart)).toBool());
    search.setSearchTerm(QStringLiteral("banana"));
    QCOMPARE(search.count(), 1);
    QVERIFY(role(search, 0, roleId(TabSearchModel::Role::GroupStart)).toBool());
    QVERIFY(changeSpy.count() >= 1);
    search.setSearchTerm(QString());
    QCOMPARE(search.count(), 4);
    QCOMPARE(role(search, 3, roleId(TabSearchModel::Role::TabId)).toInt(), d);
    QCOMPARE(resetSpy.count(), 0);

    model.closeTabById(d);
    QVERIFY(resetSpy.count() >= 1);
    QCOMPARE(search.count(), 3);
}

void tst_tabmodel::searchTakesEveryWord()
{
    TabModel model(nullptr);
    TabSearchModel search(&model);
    const int news = model.newTab(QStringLiteral("https://yle.fi/uutiset"));
    model.updateTitle(news, QStringLiteral("Helsinki news"));
    const int weather = model.newTab(QStringLiteral("https://weather.example/helsinki"));
    model.updateTitle(weather, QStringLiteral("Forecast"));
    const int phones = model.newTab(QStringLiteral("https://shop.example/"));
    model.updateTitle(phones, QStringLiteral("Älypuhelimet"));

    search.setSearchTerm(QStringLiteral("news  yle"));
    QCOMPARE(search.count(), 1);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), news);
    search.setSearchTerm(QStringLiteral("helsinki"));
    QCOMPARE(search.count(), 2);
    search.setSearchTerm(QStringLiteral("helsinki forecast"));
    QCOMPARE(search.count(), 1);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), weather);
    search.setSearchTerm(QStringLiteral("helsinki tampere"));
    QCOMPARE(search.count(), 0);
    search.setSearchTerm(QStringLiteral("ÄLY"));
    QCOMPARE(search.count(), 1);
    QCOMPARE(role(search, 0, roleId(TabSearchModel::Role::TabId)).toInt(), phones);
}

void tst_tabmodel::tabIdForUrl()
{
    TabModel model(nullptr);
    QCOMPARE(model.tabIdForUrl(QStringLiteral("https://a.example/")), 0);
    const int first = model.newTab(QStringLiteral("https://a.example/"));
    const int other = model.newTab(QStringLiteral("https://b.example/"));
    model.addGroup(QStringLiteral("Work"));
    const int second = model.newTab(QStringLiteral("https://a.example/"));
    QCOMPARE(model.tabIdForUrl(QStringLiteral("https://b.example/")), other);
    QCOMPARE(model.tabIdForUrl(QStringLiteral("https://a.example/")), second);
    model.activateTabById(first);
    QCOMPARE(model.tabIdForUrl(QStringLiteral("https://a.example/")), first);
    QCOMPARE(model.tabIdForUrl(QStringLiteral("https://c.example/")), 0);
    QCOMPARE(model.tabIdForUrl(QString()), 0);
    model.updateUrl(first, QStringLiteral("https://a.example/next"));
    QCOMPARE(model.tabIdForUrl(QStringLiteral("https://a.example/")), second);
    QCOMPARE(model.tabIdForUrl(QStringLiteral("https://a.example/next")), first);
    model.closeTabById(second);
    QCOMPARE(model.tabIdForUrl(QStringLiteral("https://a.example/")), 0);
}

void tst_tabmodel::closedTabsCanBeReopened()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    {
        TabModel model(&persistence);
        ClosedTabModel *closed = model.closedTabs();
        QVERIFY(closed != nullptr);
        QCOMPARE(closed->count(), 0);
        QCOMPARE(closed->rowCount(closed->index(0, 0)), 0);
        QSignalSpy countSpy(closed, &ClosedTabModel::countChanged);

        const int a = model.newTab(QStringLiteral("https://a.example/"));
        model.updateTitle(a, QStringLiteral("Alpha"));
        model.updateFavicon(a, QStringLiteral("https://a.example/favicon.ico"));
        const int b = model.newTab(QStringLiteral("https://b.example/"));
        model.newTab(QStringLiteral("https://c.example/"));

        model.closeTabById(a);
        model.closeTabById(b);
        QCOMPARE(closed->count(), 2);
        QCOMPARE(countSpy.count(), 2);
        QCOMPARE(role(*closed, 0, roleId(ClosedTabModel::Role::Url)).toString(),
                 QStringLiteral("https://b.example/"));
        QCOMPARE(role(*closed, 1, roleId(ClosedTabModel::Role::Url)).toString(),
                 QStringLiteral("https://a.example/"));
        QCOMPARE(role(*closed, 1, roleId(ClosedTabModel::Role::Title)).toString(),
                 QStringLiteral("Alpha"));
        QCOMPARE(role(*closed, 1, roleId(ClosedTabModel::Role::Favicon)).toString(),
                 QStringLiteral("https://a.example/favicon.ico"));
        QVERIFY(role(*closed, 1, roleId(ClosedTabModel::Role::ClosedId)).toInt() > 0);
        QVERIFY(!role(*closed, 2, roleId(ClosedTabModel::Role::Url)).isValid());
        QCOMPARE(closed->roleNames().value(roleId(ClosedTabModel::Role::ClosedId)),
                 QByteArrayLiteral("closedId"));

        closed->reopen(1);
        QCOMPARE(closed->count(), 1);
        QCOMPARE(model.activeUrl(), QStringLiteral("https://a.example/"));
        QCOMPARE(model.activeTitle(), QStringLiteral("Alpha"));
        QCOMPARE(model.activeFavicon(), QStringLiteral("https://a.example/favicon.ico"));
        QCOMPARE(model.currentGroupId(), model.defaultGroupId());
        closed->reopen(5);
        QCOMPARE(closed->count(), 1);

        model.closeAllTabs();
        QCOMPARE(closed->count(), 3);
        QCOMPARE(role(*closed, 0, roleId(ClosedTabModel::Role::Url)).toString(),
                 QStringLiteral("https://a.example/"));
    }
    {
        TabModel model(&persistence);
        ClosedTabModel *closed = model.closedTabs();
        QCOMPARE(closed->count(), 3);
        QCOMPARE(role(*closed, 2, roleId(ClosedTabModel::Role::Url)).toString(),
                 QStringLiteral("https://b.example/"));
        closed->clear();
        closed->clear();
        QCOMPARE(closed->count(), 0);
    }
    {
        TabModel model(&persistence);
        QCOMPARE(model.closedTabs()->count(), 0);
        for (int i = 0; i < ClosedTabModel::Limit + 5; ++i) {
            model.closeTabById(model.newTab(QStringLiteral("https://n%1.example/").arg(i)));
        }
        QCOMPARE(model.closedTabs()->count(), ClosedTabModel::Limit);
        QCOMPARE(role(*model.closedTabs(), 0, roleId(ClosedTabModel::Role::Url)).toString(),
                 QStringLiteral("https://n%1.example/").arg(ClosedTabModel::Limit + 4));
        TabModel again(&persistence);
        QCOMPARE(again.closedTabs()->count(), ClosedTabModel::Limit);
    }
}

void tst_tabmodel::livePagesAreCapped()
{
    TabModel model(nullptr);
    QCOMPARE(model.liveTabLimit(), 0);
    QCOMPARE(model.roleNames().value(roleId(TabModel::Role::Live)), QByteArrayLiteral("liveTab"));
    QList<int> ids;
    for (int i = 0; i < 6; ++i) {
        ids.append(model.newTab(QStringLiteral("https://t%1.example/").arg(i)));
    }
    for (int i = 0; i < 6; ++i) {
        QVERIFY(role(model, i, roleId(TabModel::Role::Live)).toBool());
    }

    QSignalSpy rowSpy(&model, &TabModel::dataChanged);
    model.setLiveTabLimit(3);
    QCOMPARE(model.liveTabLimit(), 3);
    QCOMPARE(rowSpy.count(), 3);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>(), QVector<int>{roleId(TabModel::Role::Live)});
    for (int i = 0; i < 6; ++i) {
        QCOMPARE(role(model, i, roleId(TabModel::Role::Live)).toBool(), i >= 3);
    }
    model.setLiveTabLimit(3);
    model.setLiveTabLimit(-1);
    QCOMPARE(model.liveTabLimit(), 0);
    model.setLiveTabLimit(3);

    model.activateTabById(ids.at(0));
    QVERIFY(role(model, 0, roleId(TabModel::Role::Live)).toBool());
    QVERIFY(!role(model, 3, roleId(TabModel::Role::Live)).toBool());
    QVERIFY(role(model, 4, roleId(TabModel::Role::Live)).toBool());
    QVERIFY(role(model, 5, roleId(TabModel::Role::Live)).toBool());

    model.closeTabById(ids.at(5));
    QVERIFY(role(model, model.indexOf(ids.at(3)), roleId(TabModel::Role::Live)).toBool());
    model.closeAllTabs();
}

void tst_tabmodel::mediaMetadataGoesWithWhatPlays()
{
    TabModel model(nullptr);
    const int behind = model.newTab(QStringLiteral("https://a.example/"));
    const int front = model.newTab(QStringLiteral("https://b.example/"));
    const TabModel::MediaMetadata symphony{QStringLiteral("Symphony No. 5"),
                                           QStringLiteral("Beethoven"),
                                           QStringLiteral("https://b.example/5.png")};
    QSignalSpy activeSpy(&model, &TabModel::activeMediaChanged);

    model.setMediaMetadata(front, symphony);
    QCOMPARE(model.mediaMetadata(front), TabModel::MediaMetadata());
    QCOMPARE(activeSpy.count(), 0);

    model.setMediaState(front, TabModel::MediaPlaying);
    activeSpy.clear();
    model.setMediaMetadata(front, symphony);
    QCOMPARE(activeSpy.count(), 1);
    QCOMPARE(model.activeMediaTitle(), symphony.title);
    QCOMPARE(model.activeMediaArtist(), symphony.artist);
    QCOMPARE(model.activeMediaArtwork(), symphony.artwork);
    model.setMediaMetadata(front, symphony);
    QCOMPARE(activeSpy.count(), 1);

    model.setMediaState(behind, TabModel::MediaPlaying);
    activeSpy.clear();
    model.setMediaMetadata(behind, {QStringLiteral("Other"), QString(), QString()});
    QCOMPARE(activeSpy.count(), 0);
    QCOMPARE(model.mediaMetadata(behind).title, QStringLiteral("Other"));
    model.activateTabById(behind);
    QCOMPARE(model.activeMediaTitle(), QStringLiteral("Other"));
    model.activateTabById(front);

    model.setMediaState(front, TabModel::NoMedia);
    QCOMPARE(model.activeMediaTitle(), QString());
    model.closeTabById(behind);
    QCOMPARE(model.mediaMetadata(behind), TabModel::MediaMetadata());
}

void tst_tabmodel::mediaFollowsThePage()
{
    TabModel model(nullptr);
    QCOMPARE(model.roleNames().value(roleId(TabModel::Role::Media)),
             QByteArrayLiteral("mediaState"));
    QCOMPARE(model.roleNames().value(roleId(TabModel::Role::Muted)), QByteArrayLiteral("muted"));
    QCOMPARE(model.activeMediaState(), static_cast<int>(TabModel::NoMedia));
    QVERIFY(!model.activeMuted());
    const int behind = model.newTab(QStringLiteral("https://a.example/"));
    const int front = model.newTab(QStringLiteral("https://b.example/"));
    QCOMPARE(role(model, 1, roleId(TabModel::Role::Media)).toInt(),
             static_cast<int>(TabModel::NoMedia));

    QSignalSpy rowSpy(&model, &TabModel::dataChanged);
    QSignalSpy groupRowSpy(model.groupTabs(), &GroupTabModel::dataChanged);
    QSignalSpy activeSpy(&model, &TabModel::activeMediaChanged);
    model.setMediaState(front, TabModel::MediaPlaying);
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>(),
             QVector<int>{roleId(TabModel::Role::Media)});
    QCOMPARE(groupRowSpy.count(), 1);
    QCOMPARE(activeSpy.count(), 1);
    QCOMPARE(model.activeMediaState(), static_cast<int>(TabModel::MediaPlaying));
    QCOMPARE(role(model, 1, roleId(TabModel::Role::Media)).toInt(),
             static_cast<int>(TabModel::MediaPlaying));
    model.setMediaState(front, TabModel::MediaPlaying);
    QCOMPARE(rowSpy.count(), 1);

    model.setMediaState(behind, TabModel::MediaPlaying);
    QCOMPARE(activeSpy.count(), 1);
    QCOMPARE(model.mediaState(behind), TabModel::MediaPlaying);
    QCOMPARE(model.shownMediaState(behind), TabModel::MediaPaused);
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Media)).toInt(),
             static_cast<int>(TabModel::MediaPaused));
    rowSpy.clear();
    model.activateTabById(behind);
    QCOMPARE(role(model, 0, roleId(TabModel::Role::Media)).toInt(),
             static_cast<int>(TabModel::MediaPlaying));
    QCOMPARE(role(model, 1, roleId(TabModel::Role::Media)).toInt(),
             static_cast<int>(TabModel::MediaPaused));
    int mediaRows = 0;
    for (const QList<QVariant> &change : rowSpy) {
        if (change.at(2).value<QVector<int>>().contains(roleId(TabModel::Role::Media))) {
            ++mediaRows;
        }
    }
    QCOMPARE(mediaRows, 2);
    QVERIFY(activeSpy.count() > 1);
    model.activateTabById(front);

    activeSpy.clear();
    model.setMuted(front, true);
    QVERIFY(model.isMuted(front));
    QVERIFY(model.activeMuted());
    QVERIFY(role(model, 1, roleId(TabModel::Role::Muted)).toBool());
    QCOMPARE(activeSpy.count(), 1);
    model.setMuted(front, true);
    QCOMPARE(activeSpy.count(), 1);
    model.setMediaState(front, TabModel::NoMedia);
    QVERIFY(model.isMuted(front));
    QCOMPARE(activeSpy.count(), 2);
    model.setMuted(behind, true);
    QCOMPARE(activeSpy.count(), 2);
    model.setMuted(behind, false);
    QVERIFY(!model.isMuted(behind));
    model.setMuted(front + 10, true);
    model.setMediaState(front + 10, TabModel::MediaPlaying);
    QVERIFY(!model.isMuted(front + 10));
    QCOMPARE(model.mediaState(front + 10), TabModel::NoMedia);

    model.setMediaState(behind, TabModel::MediaPaused);
    model.setLiveTabLimit(1);
    QCOMPARE(model.mediaState(behind), TabModel::NoMedia);
    model.setMediaState(behind, TabModel::MediaPlaying);
    QCOMPARE(model.mediaState(behind), TabModel::NoMedia);
    model.setLiveTabLimit(0);

    model.setMediaState(front, TabModel::MediaPlaying);
    model.closeTabById(front);
    QCOMPARE(model.mediaState(front), TabModel::NoMedia);
    QVERIFY(!model.isMuted(front));
    model.setMuted(behind, true);
    model.setMediaState(behind, TabModel::MediaPlaying);
    model.closeAllTabs();
    QCOMPARE(model.mediaState(behind), TabModel::NoMedia);
    QVERIFY(!model.isMuted(behind));
    QCOMPARE(model.activeMediaState(), static_cast<int>(TabModel::NoMedia));
}

// Url-less tab = start page: no page, no live-slot, not visit, not reopenable; showStartPage
// returns opened page to start page.
void tst_tabmodel::startPageTabs()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    TabPersistence persistence(storage);
    const QString previews = dir.path() + QStringLiteral("/previews");
    TabModel model(&persistence, previews);
    QSignalSpy visited(&model, &TabModel::visited);
    const int page = model.newTab(QStringLiteral("https://a.example/"));
    const int start = model.newTab(QString());
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.activeTabId(), start);
    QVERIFY(model.activeUrl().isEmpty());
    QCOMPARE(visited.count(), 0);

    model.setLiveTabLimit(1);
    QVERIFY(role(model, model.indexOf(page), roleId(TabModel::Role::Live)).toBool());
    QVERIFY(role(model, model.indexOf(start), roleId(TabModel::Role::Live)).toBool());

    model.updateUrl(start, QStringLiteral("https://b.example/"));
    QCOMPARE(visited.count(), 1);
    QCOMPARE(model.activeUrl(), QStringLiteral("https://b.example/"));
    QVERIFY(role(model, model.indexOf(start), roleId(TabModel::Role::Live)).toBool());
    QVERIFY(!role(model, model.indexOf(page), roleId(TabModel::Role::Live)).toBool());
    model.updateTitle(start, QStringLiteral("B"));
    model.updateFavicon(start, QStringLiteral("https://b.example/icon.png"));
    const QString preview = model.thumbnailPath(start);
    QVERIFY(writeFile(preview));
    model.updateThumbnail(start, preview);
    model.setMediaState(start, TabModel::MediaPlaying);

    QSignalSpy activeData(&model, &TabModel::activeTabDataChanged);
    QSignalSpy recent(&model, &TabModel::recentTabsChanged);
    model.showStartPage(start);
    QVERIFY(model.activeUrl().isEmpty());
    QVERIFY(model.activeTitle().isEmpty());
    QVERIFY(model.activeFavicon().isEmpty());
    QVERIFY(
        role(model, model.indexOf(start), roleId(TabModel::Role::Thumbnail)).toString().isEmpty());
    QVERIFY(!QFile::exists(preview));
    QCOMPARE(model.mediaState(start), TabModel::NoMedia);
    QVERIFY(role(model, model.indexOf(page), roleId(TabModel::Role::Live)).toBool());
    QCOMPARE(activeData.count(), 1);
    QCOMPARE(recent.count(), 1);
    QCOMPARE(visited.count(), 1);
    model.showStartPage(start);
    model.showStartPage(start + 10);
    QCOMPARE(activeData.count(), 1);

    {
        TabModel restored(&persistence, previews);
        QCOMPARE(restored.count(), 2);
        QCOMPARE(restored.activeTabId(), start);
        QVERIFY(restored.activeUrl().isEmpty());
        QVERIFY(restored.activeTitle().isEmpty());
    }

    model.closeTabById(start);
    QCOMPARE(model.closedTabs()->count(), 0);
    model.closeTabById(page);
    QCOMPARE(model.closedTabs()->count(), 1);
}

void tst_tabmodel::sharedLinksOpenInTheDefaultGroup()
{
    TabModel model(nullptr);
    model.newTab(QStringLiteral("https://home.example/"));
    const int work = model.addGroup(QStringLiteral("Work"));
    model.newTab(QStringLiteral("https://work.example/"));
    QCOMPARE(model.currentGroupId(), work);

    const int shared = model.newTabInDefaultGroup(QStringLiteral("https://shared.example/"));
    QVERIFY(shared > 0);
    QCOMPARE(model.currentGroupId(), model.defaultGroupId());
    QCOMPARE(model.activeTabId(), shared);
    QCOMPARE(model.activeUrl(), QStringLiteral("https://shared.example/"));
    QCOMPARE(model.tabCountInGroup(model.defaultGroupId()), 2);
    QCOMPARE(model.tabCountInGroup(work), 1);
    QCOMPARE(groupTabIds(*model.groupTabs()).last(), shared);

    model.setCurrentGroupId(work);
    QCOMPARE(model.newTabInDefaultGroup(QStringLiteral("tel:+358401234567")), 0);
    QCOMPARE(model.currentGroupId(), work);
}

QTEST_GUILESS_MAIN(tst_tabmodel)
#include "tst_tabmodel.moc"
