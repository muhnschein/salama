// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "bookmarks/BookmarkModel.h"
#include "history/HistoryModel.h"
#include "settings/SearchSettings.h"
#include "startpage/SiteListModel.h"
#include "startpage/StartPage.h"
#include "storage/Storage.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::BookmarkModel;
using Salama::HistoryModel;
using Salama::SearchSettings;
using Salama::Site;
using Salama::SiteListModel;
using Salama::StartPage;
using Salama::Storage;

class tst_startpage : public QObject
{
    Q_OBJECT

private slots:
    void siteList();
    void siteOf_data();
    void siteOf();
    void topSitesAreOnePerSite();
    void searchesAreNotVisits();
    void recentPagesNewestFirst();
    void bookmarksInTheirOrder();
    void listsAreShort();
};

namespace {

QStringList urls(const SiteListModel &model)
{
    QStringList list;
    for (int row = 0; row < model.rowCount(); ++row) {
        list.append(model.data(model.index(row, 0), roleId(SiteListModel::Role::Url)).toString());
    }
    return list;
}

void visit(HistoryModel &history, const QString &url, int times)
{
    for (int i = 0; i < times; ++i) {
        history.visit(url);
    }
}

} // namespace

void tst_startpage::siteList()
{
    SiteListModel model;
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.roleNames().value(roleId(SiteListModel::Role::Url)), QByteArrayLiteral("url"));
    QCOMPARE(model.roleNames().value(roleId(SiteListModel::Role::Title)),
             QByteArrayLiteral("title"));
    QCOMPARE(model.roleNames().value(roleId(SiteListModel::Role::Favicon)),
             QByteArrayLiteral("favicon"));

    QSignalSpy resetSpy(&model, &SiteListModel::modelReset);
    QSignalSpy countSpy(&model, &SiteListModel::countChanged);
    const Site a{QStringLiteral("https://a.example/"), QStringLiteral("A"),
                 QStringLiteral("https://a.example/icon.png")};
    const Site b{QStringLiteral("https://b.example/"), QString(), QString()};
    model.setSites({a, b});
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), roleId(SiteListModel::Role::Title)).toString(),
             QStringLiteral("A"));
    QCOMPARE(model.data(model.index(0, 0), roleId(SiteListModel::Role::Favicon)).toString(),
             QStringLiteral("https://a.example/icon.png"));
    QVERIFY(model.data(model.index(1, 0), roleId(SiteListModel::Role::Title)).toString().isEmpty());
    QVERIFY(!model.data(model.index(0, 0), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(model.index(2, 0), roleId(SiteListModel::Role::Url)).isValid());

    // The same list again is no change; the same sites under another title is one, of
    // as many rows.
    model.setSites({a, b});
    QCOMPARE(resetSpy.count(), 1);
    Site renamed = a;
    renamed.title = QStringLiteral("Alpha");
    QVERIFY(renamed != a);
    model.setSites({renamed, b});
    QCOMPARE(resetSpy.count(), 2);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(model.sites().first().title, QStringLiteral("Alpha"));
}

void tst_startpage::siteOf_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("site");

    QTest::newRow("root") << QStringLiteral("https://example.org/")
                          << QStringLiteral("example.org");
    QTest::newRow("www") << QStringLiteral("https://www.example.org/a?b")
                         << QStringLiteral("example.org");
    QTest::newRow("subdomain") << QStringLiteral("http://docs.example.org:8080/")
                               << QStringLiteral("docs.example.org");
    QTest::newRow("no host") << QStringLiteral("file:///tmp/a.html")
                             << QStringLiteral("file:///tmp/a.html");
}

void tst_startpage::siteOf()
{
    QFETCH(QString, url);
    QFETCH(QString, site);
    QCOMPARE(StartPage::siteOf(url), site);
}

// Visited most first, and of the pages of one site only the one visited most; the
// ones visited as often, the one visited last first.
void tst_startpage::topSitesAreOnePerSite()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel history(storage);
    visit(history, QStringLiteral("https://a.example/"), 3);
    visit(history, QStringLiteral("https://a.example/page"), 1);
    visit(history, QStringLiteral("https://www.b.example/news"), 2);
    visit(history, QStringLiteral("https://b.example/"), 1);
    visit(history, QStringLiteral("https://c.example/"), 1);
    visit(history, QStringLiteral("file:///tmp/d.html"), 1);
    history.updateTitle(QStringLiteral("https://a.example/"), QStringLiteral("Alpha"));
    history.updateFavicon(QStringLiteral("https://a.example/"),
                          QStringLiteral("https://a.example/icon.png"));

    StartPage start(storage);
    const SiteListModel &top = *start.topSites();
    QCOMPARE(urls(top), QStringList({QStringLiteral("https://a.example/"),
                                     QStringLiteral("https://www.b.example/news"),
                                     QStringLiteral("file:///tmp/d.html"),
                                     QStringLiteral("https://c.example/")}));
    QCOMPARE(top.sites().first().title, QStringLiteral("Alpha"));
    QCOMPARE(top.sites().first().favicon, QStringLiteral("https://a.example/icon.png"));
    QVERIFY(top.sites().at(1).favicon.isEmpty());

    // Read again, it follows the history.
    visit(history, QStringLiteral("https://c.example/"), 3);
    start.refresh();
    QCOMPARE(urls(top).first(), QStringLiteral("https://c.example/"));
    history.clear();
    start.refresh();
    QCOMPARE(top.count(), 0);
}

// Searching makes a page of results a visit to the engine each time; none of them is
// a site visited, or a page read.
void tst_startpage::searchesAreNotVisits()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel history(storage);
    QSettings file(dir.path() + QStringLiteral("/salama.conf"), QSettings::IniFormat);
    SearchSettings settings(file);
    for (int i = 0; i < 5; ++i) {
        history.visit(settings.searchUrl(QStringLiteral("search %1").arg(i)));
    }
    visit(history, settings.searchUrl(QStringLiteral("again")), 3);
    visit(history, QStringLiteral("https://a.example/"), 1);

    StartPage start(storage);
    QCOMPARE(urls(*start.topSites()), QStringList{QStringLiteral("https://a.example/")});
    QCOMPARE(urls(*start.recentPages()), QStringList{QStringLiteral("https://a.example/")});
}

// The pages read last, newest first, a site as often as it was read.
void tst_startpage::recentPagesNewestFirst()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel history(storage);
    visit(history, QStringLiteral("https://a.example/one"), 4);
    visit(history, QStringLiteral("https://b.example/"), 1);
    visit(history, QStringLiteral("https://a.example/two"), 1);

    StartPage start(storage);
    QCOMPARE(urls(*start.recentPages()), QStringList({QStringLiteral("https://a.example/two"),
                                                      QStringLiteral("https://b.example/"),
                                                      QStringLiteral("https://a.example/one")}));
    history.visit(QStringLiteral("https://a.example/one"));
    start.refresh();
    QCOMPARE(urls(*start.recentPages()).first(), QStringLiteral("https://a.example/one"));
}

void tst_startpage::bookmarksInTheirOrder()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    BookmarkModel bookmarks(storage);
    bookmarks.add(QStringLiteral("https://b.example/"), QStringLiteral("B"),
                  QStringLiteral("https://b.example/icon.png"));
    bookmarks.add(QStringLiteral("https://a.example/"), QStringLiteral("A"));

    StartPage start(storage);
    const SiteListModel &marked = *start.bookmarks();
    QCOMPARE(urls(marked), QStringList({QStringLiteral("https://b.example/"),
                                        QStringLiteral("https://a.example/")}));
    QCOMPARE(marked.sites().first().title, QStringLiteral("B"));
    QCOMPARE(marked.sites().first().favicon, QStringLiteral("https://b.example/icon.png"));

    bookmarks.remove(0);
    start.refresh();
    QCOMPARE(urls(marked), QStringList{QStringLiteral("https://a.example/")});
}

// Two rows of tiles, and a handful of rows: the start page is not the history, or the
// bookmarks, which have pages of their own.
void tst_startpage::listsAreShort()
{
    QTemporaryDir dir;
    Storage storage(dir.path());
    HistoryModel history(storage);
    BookmarkModel bookmarks(storage);
    for (int i = 0; i < 12; ++i) {
        const QString url = QStringLiteral("https://site%1.example/").arg(i);
        history.visit(url);
        bookmarks.add(url, QString());
    }

    StartPage start(storage);
    QCOMPARE(start.topSites()->count(), int(StartPage::TopSiteLimit));
    QCOMPARE(start.bookmarks()->count(), int(StartPage::BookmarkLimit));
    QCOMPARE(start.recentPages()->count(), int(StartPage::RecentPageLimit));
    QCOMPARE(urls(*start.bookmarks()).first(), QStringLiteral("https://site0.example/"));
    QCOMPARE(urls(*start.recentPages()).first(), QStringLiteral("https://site11.example/"));
}

QTEST_GUILESS_MAIN(tst_startpage)
#include "tst_startpage.moc"
