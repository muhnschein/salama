// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#include "storage/Storage.h"
#include "tabs/TabModel.h"
#include "tabs/TabPersistence.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Tuuli::Storage;
using Tuuli::TabModel;
using Tuuli::TabPersistence;

class tst_tabmodel : public QObject
{
    Q_OBJECT

private slots:
    void emptyModel();
    void newTabAppendsAndActivates();
    void newTabRejectsExternalUrls();
    void activation();
    void moveTabReorders();
    void closeTabActivatesPrevious();
    void closeAllTabs();
    void urlUpdatesAndVisits();
    void titleAndFavicon();
    void privateTabsStayQuiet();
    void persistenceRoundTrip();
    void thumbnailsAreCapturedPerTab();
    void thumbnailsFollowTabLifetime();
    void thumbnailsAreOptional();
    void recentThumbnailsFollowTheFront();
    void recentOrderSurvivesARestart();
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

} // namespace

void tst_tabmodel::emptyModel()
{
    TabModel model(nullptr);
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);
    QCOMPARE(model.activeTabIndex(), -1);
    QCOMPARE(model.activeTabId(), 0);
    QVERIFY(!model.activeIsPrivate());
    QVERIFY(model.activeUrl().isEmpty());
    QVERIFY(model.activeTitle().isEmpty());
    QVERIFY(model.activeFavicon().isEmpty());
    QVERIFY(!model.data(model.index(0, 0), TabModel::UrlRole).isValid());
    QCOMPARE(model.roleNames().value(TabModel::PrivateRole), QByteArrayLiteral("privateTab"));
    QCOMPARE(model.roleNames().value(TabModel::ActiveRole), QByteArrayLiteral("activeTab"));
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
    const int second = model.newTab(QStringLiteral("https://two.example/"), true);
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
    QVERIFY(model.activeIsPrivate());
    QCOMPARE(model.activeUrl(), QStringLiteral("https://two.example/"));
    QCOMPARE(role(model, 0, TabModel::TabIdRole).toInt(), first);
    QCOMPARE(role(model, 0, TabModel::UrlRole).toString(), QStringLiteral("https://one.example/"));
    QCOMPARE(role(model, 0, TabModel::ActiveRole).toBool(), false);
    QCOMPARE(role(model, 1, TabModel::ActiveRole).toBool(), true);
    QCOMPARE(role(model, 1, TabModel::PrivateRole).toBool(), true);
    QVERIFY(role(model, 1, TabModel::TitleRole).toString().isEmpty());
    QVERIFY(role(model, 1, TabModel::FaviconRole).toString().isEmpty());
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

void tst_tabmodel::activation()
{
    TabModel model(nullptr);
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    const int b = model.newTab(QStringLiteral("https://b.example/"));
    QSignalSpy activeSpy(&model, &TabModel::activeTabChanged);
    QSignalSpy rowSpy(&model, &TabModel::dataChanged);

    model.activateTab(0);
    QCOMPARE(model.activeTabId(), a);
    QCOMPARE(activeSpy.count(), 1);
    QCOMPARE(rowSpy.count(), 2);

    model.activateTab(0);
    QCOMPARE(activeSpy.count(), 1);

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

    // Carried to the end: the rest close up behind it.
    model.moveTab(0, 2);
    QCOMPARE(movedSpy.count(), 1);
    QCOMPARE(role(model, 0, TabModel::TabIdRole).toInt(), second);
    QCOMPARE(role(model, 1, TabModel::TabIdRole).toInt(), third);
    QCOMPARE(role(model, 2, TabModel::TabIdRole).toInt(), first);
    // The tab that moved is the same tab, and still the active one.
    QCOMPARE(model.activeTabId(), first);
    QCOMPARE(model.activeTabIndex(), 2);
    QCOMPARE(activeSpy.count(), 1);
    QVERIFY(role(model, 2, TabModel::ActiveRole).toBool());

    // And back towards the front.
    model.moveTab(2, 1);
    QCOMPARE(role(model, 1, TabModel::TabIdRole).toInt(), first);
    QCOMPARE(model.activeTabIndex(), 1);

    // Nothing to do, nothing reported.
    movedSpy.clear();
    model.moveTab(1, 1);
    model.moveTab(-1, 0);
    model.moveTab(0, 3);
    model.moveTab(3, 0);
    QCOMPARE(movedSpy.count(), 0);
    QCOMPARE(model.count(), 3);

    // The order is the one a restart reads back.
    const QList<Tuuli::Tab> stored = persistence.loadTabs();
    QCOMPARE(stored.count(), 3);
    QCOMPARE(stored.at(0).id, second);
    QCOMPARE(stored.at(1).id, first);
    QCOMPARE(stored.at(2).id, third);

    // A tab opened afterwards still lands last.
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

    // The engine reporting the requested url is the first visit, reported once.
    model.updateUrl(a, QStringLiteral("https://a.example/"));
    QCOMPARE(visitedSpy.count(), 1);
    model.updateUrl(a, QStringLiteral("https://a.example/"));
    QCOMPARE(visitedSpy.count(), 1);
    QCOMPARE(dataSpy.count(), 0);

    // Navigation within a tab.
    model.updateUrl(a, QStringLiteral("https://a.example/next"));
    QCOMPARE(visitedSpy.count(), 2);
    QCOMPARE(visitedSpy.last().first().toString(), QStringLiteral("https://a.example/next"));
    QCOMPARE(role(model, 0, TabModel::UrlRole).toString(),
             QStringLiteral("https://a.example/next"));
    QCOMPARE(dataSpy.count(), 0);

    // Active tab reports through activeTabDataChanged too.
    model.updateUrl(b, QStringLiteral("https://b.example/"));
    QCOMPARE(dataSpy.count(), 1);
    QCOMPARE(model.activeUrl(), QStringLiteral("https://b.example/"));

    // Ignored inputs.
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
    QCOMPARE(role(model, 0, TabModel::TitleRole).toString(), QStringLiteral("A"));
    QCOMPARE(dataSpy.count(), 0);
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>(), QVector<int>{TabModel::TitleRole});

    model.updateTitle(b, QStringLiteral("B"));
    QCOMPARE(model.activeTitle(), QStringLiteral("B"));
    QCOMPARE(dataSpy.count(), 1);

    model.updateFavicon(a, QStringLiteral("https://a.example/favicon.ico"));
    model.updateFavicon(a, QStringLiteral("https://a.example/favicon.ico"));
    model.updateFavicon(999, QStringLiteral("x"));
    QCOMPARE(faviconSpy.count(), 1);
    QCOMPARE(role(model, 0, TabModel::FaviconRole).toString(),
             QStringLiteral("https://a.example/favicon.ico"));
    model.updateFavicon(b, QStringLiteral("https://b.example/icon.png"));
    QCOMPARE(model.activeFavicon(), QStringLiteral("https://b.example/icon.png"));
    QCOMPARE(dataSpy.count(), 2);
}

void tst_tabmodel::privateTabsStayQuiet()
{
    TabModel model(nullptr);
    QSignalSpy visitedSpy(&model, &TabModel::visited);
    QSignalSpy titleSpy(&model, &TabModel::titleUpdated);
    QSignalSpy faviconSpy(&model, &TabModel::faviconUpdated);
    const int p = model.newTab(QStringLiteral("https://secret.example/"), true);

    model.updateUrl(p, QStringLiteral("https://secret.example/"));
    model.updateUrl(p, QStringLiteral("https://secret.example/more"));
    model.updateTitle(p, QStringLiteral("Secret"));
    model.updateFavicon(p, QStringLiteral("https://secret.example/favicon.ico"));

    QCOMPARE(visitedSpy.count(), 0);
    QCOMPARE(titleSpy.count(), 0);
    QCOMPARE(faviconSpy.count(), 0);
    QCOMPARE(model.activeUrl(), QStringLiteral("https://secret.example/more"));
    QCOMPARE(model.activeTitle(), QStringLiteral("Secret"));
}

void tst_tabmodel::recentThumbnailsFollowTheFront()
{
    QTemporaryDir dir;
    TabModel model(nullptr, dir.path());
    const int first = model.newTab(QStringLiteral("https://first.example/"));
    const int second = model.newTab(QStringLiteral("https://second.example/"));
    const int third = model.newTab(QStringLiteral("https://third.example/"));
    QSignalSpy recent(&model, &TabModel::recentTabsChanged);

    // One entry per tab, whether or not it has a picture, so the list is never shorter
    // than the number the cover prints above it.
    QCOMPARE(model.recentThumbnails().count(), 3);
    for (const QString &thumbnail : model.recentThumbnails()) {
        QVERIFY(thumbnail.isEmpty());
    }

    const QString firstShot = model.thumbnailPath(first);
    QVERIFY(writeFile(firstShot));
    model.updateThumbnail(first, firstShot);
    const QString thirdShot = model.thumbnailPath(third);
    QVERIFY(writeFile(thirdShot));
    model.updateThumbnail(third, thirdShot);
    QVERIFY(recent.count() > 0);

    // The third tab is the one in front: newTab activates what it opens.
    QCOMPARE(model.recentThumbnails().first(), thirdShot);

    recent.clear();
    model.activateTabById(first);
    QVERIFY(recent.count() > 0);
    QCOMPARE(model.recentThumbnails().first(), firstShot);

    // Nothing the grid does reorders the cover: a carried cell changes positions, not
    // which tab was last read.
    model.moveTab(0, 2);
    QCOMPARE(model.recentThumbnails().first(), firstShot);

    // The tab that has never been in front keeps the grid's order rather than an
    // arbitrary one: second was opened before third and comes after it here only
    // because third was activated later.
    model.activateTabById(second);
    QCOMPARE(model.recentThumbnails().at(1), firstShot);
    QCOMPARE(model.recentThumbnails().at(2), thirdShot);

    model.closeTab(model.indexOf(first));
    QCOMPARE(model.recentThumbnails().count(), 2);
    QVERIFY(!model.recentThumbnails().contains(firstShot));
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
        QCOMPARE(model.recentThumbnails().count(), 3);
        // The restored tab is in front, and was also the last one read.
        QCOMPARE(model.recentThumbnails().first(), wanted);
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
        model.newTab(QStringLiteral("https://secret.example/"), true);
        otherId = model.newTab(QStringLiteral("https://other.example/"));
        model.updateTitle(publicId, QStringLiteral("Public"));
        model.updateFavicon(publicId, QStringLiteral("https://public.example/favicon.ico"));
        model.updateUrl(otherId, QStringLiteral("https://other.example/moved"));
        model.activateTabById(publicId);
    }
    {
        TabModel model(&persistence);
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.activeTabId(), publicId);
        QCOMPARE(model.activeTitle(), QStringLiteral("Public"));
        QCOMPARE(model.activeFavicon(), QStringLiteral("https://public.example/favicon.ico"));
        QCOMPARE(role(model, 1, TabModel::UrlRole).toString(),
                 QStringLiteral("https://other.example/moved"));
        QVERIFY(!role(model, 1, TabModel::PrivateRole).toBool());

        // Restored tabs are not "visited" again when the engine reports their url.
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
        // A stale active id falls back to the first tab.
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
    const int secret = model.newTab(QStringLiteral("https://secret.example/"), true);
    QSignalSpy rowSpy(&model, &TabModel::dataChanged);

    // Private tabs and unknown tabs are never given a file to write.
    QVERIFY(model.thumbnailPath(secret).isEmpty());
    QVERIFY(model.thumbnailPath(4242).isEmpty());

    const QString first = model.thumbnailPath(a);
    QVERIFY(!first.isEmpty());
    QCOMPARE(QFileInfo(first).absolutePath(),
             QDir(dir.path() + QStringLiteral("/previews")).absolutePath());
    // Each capture gets its own name, so a new image is never hidden behind a cached one.
    QVERIFY(model.thumbnailPath(a) != first);

    QVERIFY(writeFile(first));
    model.updateThumbnail(a, first);
    QCOMPARE(role(model, 0, TabModel::ThumbnailRole).toString(), first);
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>(), QVector<int>{TabModel::ThumbnailRole});
    QCOMPARE(model.roleNames().value(TabModel::ThumbnailRole), QByteArrayLiteral("thumbnail"));

    // Replacing a preview removes the file it replaces.
    const QString second = model.thumbnailPath(a);
    QVERIFY(writeFile(second));
    model.updateThumbnail(a, second);
    QVERIFY(!QFile::exists(first));
    QVERIFY(QFile::exists(second));

    model.updateThumbnail(a, second);
    QCOMPARE(rowSpy.count(), 2);

    // A private tab keeps no preview even when one is offered.
    model.updateThumbnail(secret, second);
    QVERIFY(role(model, 1, TabModel::ThumbnailRole).toString().isEmpty());
    QVERIFY(QFile::exists(second));

    // Nothing outside the preview directory is ever removed.
    const QString outside = dir.path() + QStringLiteral("/keep.png");
    QVERIFY(writeFile(outside));
    model.updateThumbnail(a, outside);
    model.updateThumbnail(a, model.thumbnailPath(a));
    QVERIFY(QFile::exists(outside));
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

        // Closing a tab takes its preview with it.
        model.closeTab(model.indexOf(a));
        QVERIFY(!QFile::exists(shotA));
        QVERIFY(QFile::exists(shotB));
        kept = shotB;
        keptId = b;
    }
    {
        // Previews survive a restart.
        TabModel model(&persistence, previews);
        QCOMPARE(model.count(), 1);
        QCOMPARE(role(model, 0, TabModel::ThumbnailRole).toString(), kept);

        model.closeAllTabs();
        QVERIFY(!QFile::exists(kept));
    }
    {
        // A path whose file has gone reads as no preview rather than a broken one.
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
    QVERIFY(role(model, 0, TabModel::ThumbnailRole).toString().isEmpty());
}

void tst_tabmodel::thumbnailsAreOptional()
{
    // No directory means no previews, which is how the unit tests above run.
    TabModel model(nullptr);
    const int a = model.newTab(QStringLiteral("https://a.example/"));
    QVERIFY(model.thumbnailPath(a).isEmpty());
    model.updateThumbnail(a, QStringLiteral("/tmp/nowhere.png"));
    QCOMPARE(role(model, 0, TabModel::ThumbnailRole).toString(),
             QStringLiteral("/tmp/nowhere.png"));
    model.closeAllTabs();
}

QTEST_GUILESS_MAIN(tst_tabmodel)
#include "tst_tabmodel.moc"
