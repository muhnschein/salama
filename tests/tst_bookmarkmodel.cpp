// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "bookmarks/BookmarkModel.h"
#include "storage/Storage.h"

#include <QDateTime>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::BookmarkModel;
using Salama::Storage;

class tst_bookmarkmodel : public QObject
{
    Q_OBJECT

private slots:
    void addAndRoles();
    void removeVariants();
    void edit();
    void favicons();
    void activeUrl();
    void persistence();
    void revisionCountsEveryChange();
    void byId();
    void matching();
    void rowsInMemory();
};

namespace {

QVariant role(const BookmarkModel &model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

} // namespace

void tst_bookmarkmodel::addAndRoles()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    QSignalSpy countSpy(&model, &BookmarkModel::countChanged);
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);

    const int first = model.add(QStringLiteral("https://a.example/"), QStringLiteral("A"),
                                QStringLiteral("https://a.example/favicon.ico"));
    const int second = model.add(QStringLiteral("https://b.example/"), QString());
    QVERIFY(first > 0);
    QVERIFY(second > first);
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 2);

    QCOMPARE(model.add(QStringLiteral("https://a.example/"), QStringLiteral("Dup")), first);
    QCOMPARE(model.add(QString(), QStringLiteral("Empty")), 0);
    QCOMPARE(model.count(), 2);

    QCOMPARE(role(model, 0, roleId(BookmarkModel::Role::BookmarkId)).toInt(), first);
    QCOMPARE(role(model, 0, roleId(BookmarkModel::Role::Url)).toString(),
             QStringLiteral("https://a.example/"));
    QCOMPARE(role(model, 0, roleId(BookmarkModel::Role::Title)).toString(), QStringLiteral("A"));
    QCOMPARE(role(model, 0, roleId(BookmarkModel::Role::Favicon)).toString(),
             QStringLiteral("https://a.example/favicon.ico"));
    QCOMPARE(role(model, 1, roleId(BookmarkModel::Role::Title)).toString(),
             QStringLiteral("https://b.example/"));
    QVERIFY(!role(model, 1, Qt::DisplayRole).isValid());
    QVERIFY(!role(model, 9, roleId(BookmarkModel::Role::Url)).isValid());
    QCOMPARE(model.roleNames().value(roleId(BookmarkModel::Role::BookmarkId)),
             QByteArrayLiteral("bookmarkId"));
    QVERIFY(model.contains(QStringLiteral("https://b.example/")));
    QVERIFY(!model.contains(QStringLiteral("https://c.example/")));
}

void tst_bookmarkmodel::removeVariants()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    model.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    model.add(QStringLiteral("https://b.example/"), QStringLiteral("B"));
    model.add(QStringLiteral("https://c.example/"), QStringLiteral("C"));

    model.remove(1);
    QCOMPARE(model.count(), 2);
    QCOMPARE(role(model, 1, roleId(BookmarkModel::Role::Title)).toString(), QStringLiteral("C"));
    model.remove(-1);
    model.remove(2);
    QCOMPARE(model.count(), 2);

    QVERIFY(model.removeByUrl(QStringLiteral("https://a.example/")));
    QVERIFY(!model.removeByUrl(QStringLiteral("https://a.example/")));
    QCOMPARE(model.count(), 1);

    model.clear();
    QCOMPARE(model.count(), 0);
}

void tst_bookmarkmodel::edit()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    model.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    QSignalSpy rowSpy(&model, &BookmarkModel::dataChanged);

    model.edit(0, QStringLiteral("https://a.example/"), QStringLiteral("A"));
    model.edit(0, QString(), QStringLiteral("A"));
    model.edit(3, QStringLiteral("https://x.example/"), QStringLiteral("X"));
    QCOMPARE(rowSpy.count(), 0);

    model.edit(0, QStringLiteral("https://a.example/"), QStringLiteral("Alpha"));
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>(),
             QVector<int>{roleId(BookmarkModel::Role::Title)});

    model.edit(0, QStringLiteral("https://alpha.example/"), QStringLiteral("Alpha!"));
    QCOMPARE(rowSpy.count(), 2);
    QCOMPARE(rowSpy.last().at(2).value<QVector<int>>().count(), 2);
    QVERIFY(model.contains(QStringLiteral("https://alpha.example/")));
    QVERIFY(!model.contains(QStringLiteral("https://a.example/")));

    BookmarkModel reloaded(storage);
    QCOMPARE(role(reloaded, 0, roleId(BookmarkModel::Role::Title)).toString(),
             QStringLiteral("Alpha!"));
}

void tst_bookmarkmodel::favicons()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    model.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    QSignalSpy rowSpy(&model, &BookmarkModel::dataChanged);

    model.updateFavicon(QStringLiteral("https://a.example/"),
                        QStringLiteral("https://a.example/i.png"));
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(role(model, 0, roleId(BookmarkModel::Role::Favicon)).toString(),
             QStringLiteral("https://a.example/i.png"));
    model.updateFavicon(QStringLiteral("https://a.example/"),
                        QStringLiteral("https://a.example/i.png"));
    model.updateFavicon(QStringLiteral("https://none.example/"), QStringLiteral("x"));
    QCOMPARE(rowSpy.count(), 1);

    BookmarkModel reloaded(storage);
    QCOMPARE(role(reloaded, 0, roleId(BookmarkModel::Role::Favicon)).toString(),
             QStringLiteral("https://a.example/i.png"));
}

void tst_bookmarkmodel::activeUrl()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    QSignalSpy urlSpy(&model, &BookmarkModel::activeUrlChanged);
    QSignalSpy bookmarkedSpy(&model, &BookmarkModel::activeUrlBookmarkedChanged);

    QVERIFY(!model.activeUrlBookmarked());
    model.setActiveUrl(QStringLiteral("https://a.example/"));
    model.setActiveUrl(QStringLiteral("https://a.example/"));
    QCOMPARE(urlSpy.count(), 1);
    QCOMPARE(model.activeUrl(), QStringLiteral("https://a.example/"));
    QVERIFY(!model.activeUrlBookmarked());

    model.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    QVERIFY(model.activeUrlBookmarked());
    QVERIFY(bookmarkedSpy.count() >= 2);

    model.removeByUrl(QStringLiteral("https://a.example/"));
    QVERIFY(!model.activeUrlBookmarked());
}

void tst_bookmarkmodel::persistence()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    {
        BookmarkModel model(storage);
        model.add(QStringLiteral("https://z.example/"), QStringLiteral("Z"));
        model.add(QStringLiteral("https://y.example/"), QStringLiteral("Y"));
    }
    BookmarkModel model(storage);
    QCOMPARE(model.count(), 2);
    QCOMPARE(role(model, 0, roleId(BookmarkModel::Role::Title)).toString(), QStringLiteral("Z"));
    QCOMPARE(role(model, 1, roleId(BookmarkModel::Role::Title)).toString(), QStringLiteral("Y"));
}

// Every change to the bookmarks moves the revision on, so a QML binding that reads it
// before asking an invokable is asked again (docs/DECISIONS/0029-quick-action.md).
void tst_bookmarkmodel::revisionCountsEveryChange()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    QSignalSpy spy(&model, &BookmarkModel::revisionChanged);
    int revision = model.revision();

    auto moved = [&]() {
        const bool on = model.revision() > revision && spy.count() == 1;
        revision = model.revision();
        spy.clear();
        return on;
    };

    model.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    QVERIFY(moved());
    model.add(QStringLiteral("https://b.example/"), QStringLiteral("B"));
    QVERIFY(moved());
    model.edit(0, QStringLiteral("https://a.example/"), QStringLiteral("Alpha"));
    QVERIFY(moved());
    model.updateFavicon(QStringLiteral("https://a.example/"), QStringLiteral("a.png"));
    QVERIFY(moved());
    model.remove(1);
    QVERIFY(moved());
    model.clear();
    QVERIFY(moved());

    // What changes nothing moves nothing.
    model.add(QStringLiteral("https://c.example/"), QStringLiteral("C"));
    QVERIFY(moved());
    model.add(QStringLiteral("https://c.example/"), QStringLiteral("C again"));
    model.edit(0, QStringLiteral("https://c.example/"), QStringLiteral("C"));
    model.updateFavicon(QStringLiteral("https://none.example/"), QStringLiteral("x.png"));
    model.remove(5);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(model.revision(), revision);
}

// One bookmark by its id, as the quick action keeps it; and the id by the address,
// which is how it is found again once removed and added back.
void tst_bookmarkmodel::byId()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    const int a = model.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));
    const int b = model.add(QStringLiteral("https://b.example/"), QString());

    QVERIFY(model.hasBookmark(a));
    QVERIFY(model.hasBookmark(b));
    QVERIFY(!model.hasBookmark(0));
    QVERIFY(!model.hasBookmark(b + 1));
    QCOMPARE(model.urlOf(a), QStringLiteral("https://a.example/"));
    QCOMPARE(model.titleOf(a), QStringLiteral("A"));
    // No title: the address, as the list shows it.
    QCOMPARE(model.titleOf(b), QStringLiteral("https://b.example/"));
    QVERIFY(model.urlOf(b + 1).isEmpty());
    QVERIFY(model.titleOf(b + 1).isEmpty());
    QCOMPARE(model.idForUrl(QStringLiteral("https://b.example/")), b);
    QCOMPARE(model.idForUrl(QStringLiteral("https://c.example/")), 0);
    QCOMPARE(model.idForUrl(QString()), 0);

    // Renamed: the id reads as it is now.
    model.edit(0, QStringLiteral("https://a.example/"), QStringLiteral("Alpha"));
    QCOMPARE(model.titleOf(a), QStringLiteral("Alpha"));

    // Removed and added again, as the menu's Bookmark toggle does: a new id, found by
    // the address.
    model.removeByUrl(QStringLiteral("https://a.example/"));
    QVERIFY(!model.hasBookmark(a));
    const int again = model.add(QStringLiteral("https://a.example/"), QStringLiteral("Alpha"));
    QVERIFY(again != a);
    QCOMPARE(model.idForUrl(QStringLiteral("https://a.example/")), again);
}

// What the bookmark picker lists: every word, anywhere, whatever the case; all of
// them for nothing typed.
void tst_bookmarkmodel::matching()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    const int news = model.add(QStringLiteral("https://yle.fi/uutiset"),
                               QStringLiteral("Helsinki news"), QStringLiteral("yle.png"));
    const int phones =
        model.add(QStringLiteral("https://shop.example/phones"), QStringLiteral("Älypuhelimet"));
    const int bare = model.add(QStringLiteral("https://bare.example/news"), QString());

    const QVariantList all = model.matching(QString());
    QCOMPARE(all.count(), 3);
    QCOMPARE(model.matching(QStringLiteral("   ")).count(), 3);
    const QVariantMap first = all.first().toMap();
    QCOMPARE(first.value(QStringLiteral("bookmarkId")).toInt(), news);
    QCOMPARE(first.value(QStringLiteral("title")).toString(), QStringLiteral("Helsinki news"));
    QCOMPARE(first.value(QStringLiteral("url")).toString(),
             QStringLiteral("https://yle.fi/uutiset"));
    QCOMPARE(first.value(QStringLiteral("favicon")).toString(), QStringLiteral("yle.png"));
    QCOMPARE(first.count(), 4);
    QCOMPARE(all.at(2).toMap().value(QStringLiteral("title")).toString(),
             QStringLiteral("https://bare.example/news"));

    // Words across title and address, in the list's order.
    QVariantList found = model.matching(QStringLiteral("NEWS"));
    QCOMPARE(found.count(), 2);
    QCOMPARE(found.at(0).toMap().value(QStringLiteral("bookmarkId")).toInt(), news);
    QCOMPARE(found.at(1).toMap().value(QStringLiteral("bookmarkId")).toInt(), bare);
    found = model.matching(QStringLiteral("news yle"));
    QCOMPARE(found.count(), 1);
    QCOMPARE(found.at(0).toMap().value(QStringLiteral("bookmarkId")).toInt(), news);
    found = model.matching(QStringLiteral("äly shop"));
    QCOMPARE(found.count(), 1);
    QCOMPARE(found.at(0).toMap().value(QStringLiteral("bookmarkId")).toInt(), phones);
    QVERIFY(model.matching(QStringLiteral("news tampere")).isEmpty());
}

// The rows as kept, for the address bar: the title as stored, not as shown.
void tst_bookmarkmodel::rowsInMemory()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel model(storage);
    QVERIFY(model.bookmarks().isEmpty());
    const int a = model.add(QStringLiteral("https://a.example/"), QStringLiteral("A"),
                            QStringLiteral("a.png"));
    const int b = model.add(QStringLiteral("https://b.example/"), QString());
    QCOMPARE(model.bookmarks().count(), 2);
    QCOMPARE(model.bookmarks().at(0).id, a);
    QCOMPARE(model.bookmarks().at(0).url, QStringLiteral("https://a.example/"));
    QCOMPARE(model.bookmarks().at(0).title, QStringLiteral("A"));
    QCOMPARE(model.bookmarks().at(0).favicon, QStringLiteral("a.png"));
    QCOMPARE(model.bookmarks().at(1).id, b);
    QVERIFY(model.bookmarks().at(1).title.isEmpty());

    // When each was added, to the second, in milliseconds: as added, and as read back.
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 created = model.bookmarks().at(0).created;
    QCOMPARE(created % 1000, qint64(0));
    QVERIFY(created <= now && now - created < qint64(60) * 1000);
    BookmarkModel reloaded(storage);
    QCOMPARE(reloaded.bookmarks().at(0).created, created);
}

QTEST_GUILESS_MAIN(tst_bookmarkmodel)
#include "tst_bookmarkmodel.moc"
