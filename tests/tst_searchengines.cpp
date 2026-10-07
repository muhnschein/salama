// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "search/SearchEngines.h"
#include "settings/SearchSettings.h"
#include "settings/SettingsSections.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::SearchEngines;
using Salama::SearchSettings;
using Sections = Salama::SettingsSections;

namespace {

const char *const FindHref = "https://find.example/opensearch.xml";
const char *const SeekHref = "https://seek.example/description.xml";

QString description(const QString &name, const QString &address)
{
    return QStringLiteral("<OpenSearchDescription xmlns=\"http://a9.com/-/spec/opensearch/1.1/\">"
                          "<ShortName>%1</ShortName>"
                          "<Url type=\"text/html\" template=\"%2\"/></OpenSearchDescription>")
        .arg(name, address);
}

QString findDescription()
{
    return description(QStringLiteral("Find"),
                       QStringLiteral("https://find.example/results?query={searchTerms}"));
}

QString seekDescription()
{
    return description(QStringLiteral("Seek"),
                       QStringLiteral("https://www.seek.example/look/{searchTerms}/"));
}

void offerBoth(SearchEngines *list)
{
    QVERIFY(list->offerEngine(QStringLiteral("Find"), QLatin1String(FindHref),
                              QStringLiteral("find.example")));
    QVERIFY(list->offerEngine(QStringLiteral("Seek"), QLatin1String(SeekHref),
                              QStringLiteral("seek.example")));
}

QStringList titlesOf(const QVariantList &offers)
{
    QStringList titles;
    for (const QVariant &offer : offers) {
        titles.append(offer.toMap().value(QStringLiteral("title")).toString());
    }
    return titles;
}

} // namespace

class tst_searchengines : public QObject
{
    Q_OBJECT

private slots:
    void builtInOnly();
    void offersAreKept();
    void offersAreNotKeptTwice_data();
    void offersAreNotKeptTwice();
    void offersArePersisted();
    void forgettingAnOffer();
    void addingAnEngine();
    void addingFails_data();
    void addingFails();
    void addedEnginesArePersisted();
    void addedEnginesUseTheirTemplates();
    void keysAreUnique();
    void removingTheEngineInUse();
    void removingAnotherEngine();
    void removingWhatCannotBeRemoved();
    void resetFallsBack();
    void resetKeepsABuiltInChoice();
    void resetOfNothingSaysNothing();
    void indexFollowsTheList();
    void lookupsByIndex();
    void anAddedEngineIsChosen();
    void anEngineAddedUnderAnUnknownKeyIsNotChosen();
    void storedGarbageIsPassedOver();
    void searchPagesOfAddedEngines();
    void typedTextWithAnAddedEngine();
};

void tst_searchengines::builtInOnly()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchEngines *list = settings.searchEngines();
    QCOMPARE(list->engineNames(), QStringList({QStringLiteral("Qwant"), QStringLiteral("Ecosia"),
                                               QStringLiteral("Startpage")}));
    QCOMPARE(list->engineKeys(), QStringList({QStringLiteral("qwant"), QStringLiteral("ecosia"),
                                              QStringLiteral("startpage")}));
    QCOMPARE(list->engineHosts(), QStringList({QString(), QString(), QString()}));
    QCOMPARE(list->addedCount(), 0);
    QVERIFY(list->foundEngines().isEmpty());
}

void tst_searchengines::offersAreKept()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchEngines *list = settings.searchEngines();
    QSignalSpy found(list, &SearchEngines::foundChanged);
    QSignalSpy engines(list, &SearchEngines::enginesChanged);

    offerBoth(list);
    QCOMPARE(found.count(), 2);
    QCOMPARE(engines.count(), 0);
    QCOMPARE(list->engineNames().count(), 3);

    const QVariantList offers = list->foundEngines();
    QCOMPARE(titlesOf(offers), QStringList({QStringLiteral("Find"), QStringLiteral("Seek")}));
    QCOMPARE(offers.first().toMap().value(QStringLiteral("href")).toString(),
             QLatin1String(FindHref));
    QCOMPARE(offers.first().toMap().value(QStringLiteral("host")).toString(),
             QStringLiteral("find.example"));
}

// Page offers every visit, maybe twice; already offered, listed or same-named not kept again.
// Host: page's, else description's.
void tst_searchengines::offersAreNotKeptTwice_data()
{
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("href");
    QTest::addColumn<bool>("kept");

    QTest::newRow("the same again") << "Find" << FindHref << false;
    QTest::newRow("same title, another address")
        << "Find" << "https://find.example/other.xml" << false;
    QTest::newRow("same title in other case") << "FIND" << "https://find.example/o.xml" << false;
    QTest::newRow("same title, spaced") << "  Find " << "https://find.example/o.xml" << false;
    QTest::newRow("same address, another title") << "Look" << FindHref << false;
    QTest::newRow("a built-in engine") << "Qwant" << "https://qwant.example/o.xml" << false;
    QTest::newRow("a built-in engine, other case")
        << "ecosia" << "https://ecosia.example/o.xml" << false;
    QTest::newRow("an engine already added") << "Added" << "https://added.example/o.xml" << false;
    QTest::newRow("no title") << "  " << "https://other.example/o.xml" << false;
    QTest::newRow("no address") << "Other" << "" << false;
    QTest::newRow("not the web") << "Other" << "ftp://other.example/o.xml" << false;
    QTest::newRow("not an address") << "Other" << "o.xml" << false;
    QTest::newRow("a new one") << "Other" << "https://other.example/o.xml" << true;
}

void tst_searchengines::offersAreNotKeptTwice()
{
    QFETCH(QString, title);
    QFETCH(QString, href);
    QFETCH(bool, kept);
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchEngines *list = settings.searchEngines();
    QVERIFY(list->offerEngine(QStringLiteral("Added"), QStringLiteral("https://added.example/o"),
                              QString()));
    QVERIFY(list->addFoundEngine(
        QStringLiteral("https://added.example/o"),
        description(QStringLiteral("Added"),
                    QStringLiteral("https://added.example/?q={searchTerms}"))));
    QVERIFY(list->offerEngine(QStringLiteral("Find"), QLatin1String(FindHref), QString()));

    QSignalSpy found(list, &SearchEngines::foundChanged);
    QCOMPARE(list->offerEngine(title, href, QString()), kept);
    QCOMPARE(found.count(), kept ? 1 : 0);
    QCOMPARE(list->foundEngines().count(), kept ? 2 : 1);
}

void tst_searchengines::offersArePersisted()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QVERIFY(settings.searchEngines()->offerEngine(QStringLiteral("Find, \"Seek\" & co"),
                                                      QLatin1String(FindHref), QString()));
        QVERIFY(settings.searchEngines()->offerEngine(
            QStringLiteral("Seek"), QLatin1String(SeekHref), QStringLiteral("seek.example")));
    }
    Sections again(path);
    const QVariantList offers = again.searchEngines()->foundEngines();
    QCOMPARE(titlesOf(offers),
             QStringList({QStringLiteral("Find, \"Seek\" & co"), QStringLiteral("Seek")}));
    QCOMPARE(offers.first().toMap().value(QStringLiteral("host")).toString(),
             QStringLiteral("find.example"));
    QCOMPARE(offers.last().toMap().value(QStringLiteral("host")).toString(),
             QStringLiteral("seek.example"));
    QVERIFY(!again.searchEngines()->offerEngine(QStringLiteral("seek"),
                                                QStringLiteral("https://x.example/"), QString()));
}

void tst_searchengines::forgettingAnOffer()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    Sections settings(path);
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QSignalSpy found(list, &SearchEngines::foundChanged);

    list->forgetFoundEngine(QStringLiteral("https://nothing.example/"));
    QCOMPARE(found.count(), 0);
    list->forgetFoundEngine(QLatin1String(FindHref));
    QCOMPARE(found.count(), 1);
    QCOMPARE(titlesOf(list->foundEngines()), QStringList{QStringLiteral("Seek")});
    QCOMPARE(titlesOf(Sections(path).searchEngines()->foundEngines()),
             QStringList{QStringLiteral("Seek")});

    QVERIFY(list->offerEngine(QStringLiteral("Find"), QLatin1String(FindHref), QString()));
    list->forgetFoundEngine(QLatin1String(FindHref));
    list->forgetFoundEngine(QLatin1String(SeekHref));
    QVERIFY(list->foundEngines().isEmpty());
    QVERIFY(Sections(path).searchEngines()->foundEngines().isEmpty());
}

void tst_searchengines::addingAnEngine()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QSignalSpy engines(list, &SearchEngines::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);
    QSignalSpy found(list, &SearchEngines::foundChanged);

    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QCOMPARE(engines.count(), 1);
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(found.count(), 1);
    QCOMPARE(list->engineNames().last(), QStringLiteral("Find"));
    QCOMPARE(list->engineNames().count(), 4);
    QCOMPARE(list->engineHosts(),
             QStringList({QString(), QString(), QString(), QStringLiteral("find.example")}));
    QCOMPARE(list->addedCount(), 1);
    QCOMPARE(titlesOf(list->foundEngines()), QStringList{QStringLiteral("Seek")});
    QCOMPARE(search->engineIndex(), 3);
    QCOMPARE(search->engine(), list->engineKeys().last());
    QCOMPARE(list->engineNames().at(search->engineIndex()), QStringLiteral("Find"));
    QCOMPARE(search->searchUrl(QStringLiteral("sailfish os")),
             QStringLiteral("https://find.example/results?query=sailfish%20os"));

    QVERIFY(list->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(list->engineNames().last(), QStringLiteral("Seek"));
    QCOMPARE(search->engineIndex(), 4);
    QVERIFY(list->foundEngines().isEmpty());
}

void tst_searchengines::addingFails_data()
{
    QTest::addColumn<QString>("href");
    QTest::addColumn<QString>("text");

    QTest::newRow("not an offer") << "https://nothing.example/o.xml" << findDescription();
    QTest::newRow("garbage") << FindHref << "<html><body>Not found";
    QTest::newRow("empty") << FindHref << "";
    QTest::newRow("post only") << FindHref
                               << QStringLiteral(
                                      "<OpenSearchDescription><ShortName>Find</ShortName>"
                                      "<Url type=\"text/html\" method=\"POST\" "
                                      "template=\"https://find.example/\"/>"
                                      "</OpenSearchDescription>");
    QTest::newRow("a built-in engine's name")
        << FindHref
        << description(QStringLiteral("qwant"),
                       QStringLiteral("https://q.example/?q={searchTerms}"));
}

void tst_searchengines::addingFails()
{
    QFETCH(QString, href);
    QFETCH(QString, text);
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    QVERIFY(list->offerEngine(QStringLiteral("Find"), QLatin1String(FindHref), QString()));
    QSignalSpy engines(list, &SearchEngines::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);
    QSignalSpy found(list, &SearchEngines::foundChanged);

    QVERIFY(!list->addFoundEngine(href, text));
    QCOMPARE(engines.count() + chosen.count() + found.count(), 0);
    QCOMPARE(list->engineNames().count(), 3);
    QCOMPARE(list->addedCount(), 0);
    QCOMPARE(search->engine(), SearchSettings::defaultEngine());
    QCOMPARE(titlesOf(list->foundEngines()), QStringList{QStringLiteral("Find")});
}

void tst_searchengines::addedEnginesArePersisted()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        offerBoth(settings.searchEngines());
        QVERIFY(
            settings.searchEngines()->addFoundEngine(QLatin1String(FindHref), findDescription()));
    }
    Sections again(path);
    SearchSettings *search = again.search();
    SearchEngines *list = again.searchEngines();
    QCOMPARE(list->engineNames().last(), QStringLiteral("Find"));
    QCOMPARE(list->engineHosts().last(), QStringLiteral("find.example"));
    QCOMPARE(search->engineIndex(), 3);
    QCOMPARE(search->searchUrl(QStringLiteral("a b")),
             QStringLiteral("https://find.example/results?query=a%20b"));
    QCOMPARE(titlesOf(list->foundEngines()), QStringList{QStringLiteral("Seek")});
    QCOMPARE(list->addedCount(), 1);
}

void tst_searchengines::addedEnginesUseTheirTemplates()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(search->searchUrl(QStringLiteral(" 100% a&b ")),
             QStringLiteral("https://www.seek.example/look/100%25%20a%26b/"));
    search->setEngineIndex(1);
    QCOMPARE(search->searchUrl(QStringLiteral("a")),
             QStringLiteral("https://www.ecosia.org/search?q=a"));
    search->setEngine(list->engineKeys().last());
    QCOMPARE(search->searchUrl(QStringLiteral("a")),
             QStringLiteral("https://www.seek.example/look/a/"));
}

void tst_searchengines::keysAreUnique()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    const QStringList names{QStringLiteral("A B"), QStringLiteral("A-B"), QStringLiteral("A.B"),
                            QStringLiteral("???")};
    for (int i = 0; i < names.count(); ++i) {
        const QString href = QStringLiteral("https://site%1.example/o.xml").arg(i);
        QVERIFY(list->offerEngine(names.at(i), href, QString()));
        QVERIFY(list->addFoundEngine(
            href, description(names.at(i),
                              QStringLiteral("https://site%1.example/?q={searchTerms}").arg(i))));
    }
    QStringList keys = list->engineKeys();
    QCOMPARE(keys.count(), 7);
    QCOMPARE(keys.removeDuplicates(), 0);
    for (int i = 0; i < keys.count(); ++i) {
        search->setEngineIndex(i);
        QCOMPARE(search->engineIndex(), i);
    }
}

void tst_searchengines::removingTheEngineInUse()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    Sections settings(path);
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    const QString key = search->engine();
    QSignalSpy engines(list, &SearchEngines::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);

    list->removeAddedEngine(key);
    QCOMPARE(engines.count(), 1);
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(list->engineNames().count(), 3);
    QCOMPARE(list->addedCount(), 0);
    QCOMPARE(search->engine(), SearchSettings::defaultEngine());
    QCOMPARE(search->engineIndex(), 0);
    QCOMPARE(QSettings(path, QSettings::IniFormat).value(QStringLiteral("searchEngine")).toString(),
             SearchSettings::defaultEngine());
    QCOMPARE(Sections(path).searchEngines()->engineNames().count(), 3);
    QCOMPARE(search->searchUrl(QStringLiteral("a")), QStringLiteral("https://www.qwant.com/?q=a"));
}

void tst_searchengines::removingAnotherEngine()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    const QString find = search->engine();
    QVERIFY(list->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    const QString seek = search->engine();

    list->removeAddedEngine(find);
    QCOMPARE(search->engine(), seek);
    QCOMPARE(list->engineNames().last(), QStringLiteral("Seek"));
    QCOMPARE(search->engineIndex(), 3);
    QCOMPARE(list->addedCount(), 1);

    search->setEngineIndex(2);
    list->removeAddedEngine(seek);
    QCOMPARE(list->engineKeys().count(), 3);
    QCOMPARE(search->engine(), QStringLiteral("startpage"));
}

void tst_searchengines::removingWhatCannotBeRemoved()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchEngines *list = settings.searchEngines();
    QSignalSpy engines(list, &SearchEngines::enginesChanged);
    list->removeAddedEngine(QStringLiteral("qwant"));
    list->removeAddedEngine(QStringLiteral("added-nothing"));
    list->removeAddedEngine(QString());
    QCOMPARE(engines.count(), 0);
    QCOMPARE(list->engineNames().count(), 3);
}

void tst_searchengines::resetFallsBack()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    Sections settings(path);
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QVERIFY(list->offerEngine(QStringLiteral("Third"), QStringLiteral("https://third.example/o"),
                              QString()));
    QCOMPARE(search->engineIndex(), 3);
    QSignalSpy engines(list, &SearchEngines::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);
    QSignalSpy found(list, &SearchEngines::foundChanged);

    list->removeAddedEngines();
    QCOMPARE(engines.count(), 1);
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(found.count(), 1);
    QCOMPARE(list->engineNames().count(), 3);
    QVERIFY(list->foundEngines().isEmpty());
    QCOMPARE(list->addedCount(), 0);
    QCOMPARE(search->engine(), SearchSettings::defaultEngine());
    QCOMPARE(search->engineIndex(), 0);
    Sections again(path);
    QCOMPARE(again.searchEngines()->engineNames().count(), 3);
    QVERIFY(again.searchEngines()->foundEngines().isEmpty());
    QCOMPARE(QSettings(path, QSettings::IniFormat).value(QStringLiteral("searchEngine")).toString(),
             SearchSettings::defaultEngine());
    QVERIFY(!QSettings(path, QSettings::IniFormat).contains(QStringLiteral("searchEnginesAdded")));
    QVERIFY(!QSettings(path, QSettings::IniFormat).contains(QStringLiteral("searchEnginesFound")));
}

void tst_searchengines::resetKeepsABuiltInChoice()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    search->setEngine(QStringLiteral("startpage"));
    QSignalSpy chosen(search, &SearchSettings::engineChanged);

    list->removeAddedEngines();
    QCOMPARE(search->engine(), QStringLiteral("startpage"));
    QCOMPARE(list->engineNames().count(), 3);
    QVERIFY(list->foundEngines().isEmpty());
    QCOMPARE(search->engineIndex(), 2);
}

void tst_searchengines::resetOfNothingSaysNothing()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    QSignalSpy engines(list, &SearchEngines::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);
    QSignalSpy found(list, &SearchEngines::foundChanged);
    list->removeAddedEngines();
    QCOMPARE(engines.count() + chosen.count() + found.count(), 0);

    offerBoth(list);
    found.clear();
    list->removeAddedEngines();
    QCOMPARE(found.count(), 1);
    QCOMPARE(engines.count() + chosen.count(), 0);
}

void tst_searchengines::indexFollowsTheList()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QVERIFY(list->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(search->engineIndex(), 4);
    search->setEngineIndex(5);
    QCOMPARE(search->engineIndex(), 4);
    search->setEngineIndex(3);
    QCOMPARE(list->engineNames().at(search->engineIndex()), QStringLiteral("Find"));
    search->setEngineIndex(0);
    QCOMPARE(search->engine(), QStringLiteral("qwant"));
}

void tst_searchengines::lookupsByIndex()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchEngines *list = settings.searchEngines();
    QCOMPARE(Salama::defaultSearchEngine(), QStringLiteral("qwant"));
    QCOMPARE(Salama::withoutWww(QStringLiteral("www.find.example")),
             QStringLiteral("find.example"));
    QCOMPARE(Salama::withoutWww(QStringLiteral("find.example")), QStringLiteral("find.example"));
    QCOMPARE(list->count(), 3);
    QCOMPARE(list->indexOf(QStringLiteral("startpage")), 2);
    QCOMPARE(list->indexOf(QStringLiteral("added-find")), -1);
    QCOMPARE(list->keyAt(1), QStringLiteral("ecosia"));
    QCOMPARE(list->templateAt(1), QStringLiteral("https://www.ecosia.org/search?q={searchTerms}"));

    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QCOMPARE(list->count(), 4);
    QCOMPARE(list->indexOf(QStringLiteral("added-find")), 3);
    QCOMPARE(list->keyAt(3), QStringLiteral("added-find"));
    QCOMPARE(list->templateAt(3),
             QStringLiteral("https://find.example/results?query={searchTerms}"));
    list->removeAddedEngine(QStringLiteral("added-find"));
    QCOMPARE(list->count(), 3);
    QCOMPARE(list->indexOf(QStringLiteral("added-find")), -1);
}

// Taken engine announced before list change, so engine in use is new one by time index is
// asked; search settings select it on that signal alone.
void tst_searchengines::anAddedEngineIsChosen()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    Sections settings(path);
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QStringList chosenAtTheTime;
    QObject::connect(list, &SearchEngines::enginesChanged,
                     [&]() { chosenAtTheTime.append(search->engine()); });
    QSignalSpy added(list, &SearchEngines::engineAdded);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);

    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QCOMPARE(added.count(), 1);
    QCOMPARE(added.first().first().toString(), QStringLiteral("added-find"));
    QCOMPARE(search->engine(), QStringLiteral("added-find"));
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(chosenAtTheTime, QStringList{QStringLiteral("added-find")});
    QCOMPARE(QSettings(path, QSettings::IniFormat).value(QStringLiteral("searchEngine")).toString(),
             QStringLiteral("added-find"));

    search->setEngine(QStringLiteral("ecosia"));
    QVERIFY(list->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(added.count(), 2);
    QCOMPARE(search->engine(), QStringLiteral("added-seek"));

    QVERIFY(list->offerEngine(QStringLiteral("Third"), QStringLiteral("https://third.example/o"),
                              QString()));
    QVERIFY(!list->addFoundEngine(QStringLiteral("https://third.example/o"), QString()));
    QCOMPARE(added.count(), 2);
    QCOMPARE(search->engine(), QStringLiteral("added-seek"));
}

void tst_searchengines::anEngineAddedUnderAnUnknownKeyIsNotChosen()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    QSignalSpy chosen(search, &SearchSettings::engineChanged);

    emit list->engineAdded(QStringLiteral("added-nothing"));
    QCOMPARE(search->engine(), SearchSettings::defaultEngine());
    QCOMPARE(chosen.count(), 0);
}

void tst_searchengines::storedGarbageIsPassedOver()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        QSettings file(path, QSettings::IniFormat);
        file.setValue(QStringLiteral("searchEnginesAdded"),
                      QStringLiteral("[{\"key\":\"added-ok\",\"name\":\"Ok\","
                                     "\"template\":\"https://ok.example/?q={searchTerms}\","
                                     "\"host\":\"ok.example\"},"
                                     "{\"key\":\"added-ok\",\"name\":\"Twice\","
                                     "\"template\":\"https://ok.example/?q={searchTerms}\"},"
                                     "{\"key\":\"qwant\",\"name\":\"Mine\","
                                     "\"template\":\"https://ok.example/?q={searchTerms}\"},"
                                     "{\"key\":\"added-bad\",\"name\":\"Bad\","
                                     "\"template\":\"javascript:{searchTerms}\"},"
                                     "{\"key\":\"added-no\",\"name\":\"No\","
                                     "\"template\":\"https://no.example/?q=%1\"},"
                                     "{\"name\":\"No key\","
                                     "\"template\":\"https://ok.example/?q={searchTerms}\"},"
                                     "{\"key\":\"added-nameless\","
                                     "\"template\":\"https://ok.example/?q={searchTerms}\"},"
                                     "7, \"text\", null]"));
        file.setValue(QStringLiteral("searchEnginesFound"),
                      QStringLiteral("[{\"title\":\"Fine\",\"href\":\"https://fine.example/o\"},"
                                     "{\"title\":\"Fine\",\"href\":\"https://fine.example/o\"},"
                                     "{\"title\":\"No address\"},"
                                     "{\"title\":\"Not web\",\"href\":\"ftp://x.example/o\"},"
                                     "{\"href\":\"https://untitled.example/o\"}]"));
        file.setValue(QStringLiteral("searchEngine"), QStringLiteral("added-ok"));
    }
    Sections settings(path);
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    QCOMPARE(list->engineNames().mid(3), QStringList{QStringLiteral("Ok")});
    QCOMPARE(search->engine(), QStringLiteral("added-ok"));
    QCOMPARE(titlesOf(list->foundEngines()), QStringList{QStringLiteral("Fine")});

    {
        QSettings file(path, QSettings::IniFormat);
        file.setValue(QStringLiteral("searchEnginesAdded"), QStringLiteral("not json {"));
        file.setValue(QStringLiteral("searchEnginesFound"), QStringLiteral("{\"title\":\"x\"}"));
    }
    Sections broken(path);
    QCOMPARE(broken.searchEngines()->engineNames().count(), 3);
    QVERIFY(broken.searchEngines()->foundEngines().isEmpty());
    QCOMPARE(broken.search()->engine(), SearchSettings::defaultEngine());
}

// Added engine result pages = searches, like built-in, whatever url shape; start page reads
// history by this (src/startpage/StartPage.cpp).
void tst_searchengines::searchPagesOfAddedEngines()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchEngines *list = settings.searchEngines();
    const QString find = QStringLiteral("https://find.example/results?query=forest");
    const QString seek = QStringLiteral("https://seek.example/look/forest/");
    QVERIFY(!list->isSearchUrl(find));
    QVERIFY(!list->isSearchUrl(seek));

    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QVERIFY(list->isSearchUrl(find));
    QVERIFY(list->isSearchUrl(QStringLiteral("https://www.find.example/results?x=1&query=a")));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://find.example/results")));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://find.example/other?query=forest")));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://find.example/")));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://elsewhere.example/results?query=a")));
    QVERIFY(!list->isSearchUrl(seek));

    QVERIFY(list->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QVERIFY(list->isSearchUrl(seek));
    QVERIFY(list->isSearchUrl(QStringLiteral("https://www.seek.example/look/two%20words/")));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://seek.example/look/")));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://seek.example/look/forest")));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://seek.example/")));
    QVERIFY(list->isSearchUrl(find));

    QVERIFY(list->isSearchUrl(QStringLiteral("https://www.qwant.com/?q=a")));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://www.qwant.com/")));

    list->removeAddedEngines();
    QVERIFY(!list->isSearchUrl(find));
    QVERIFY(!list->isSearchUrl(seek));
    QVERIFY(list->isSearchUrl(QStringLiteral("https://www.qwant.com/?q=a")));
}

// Typed text searched with engine in use, added or not; addresses stay addresses. Engine
// with words as whole path still searches, but results not read (start page would take
// whole site for search).
void tst_searchengines::typedTextWithAnAddedEngine()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    SearchEngines *list = settings.searchEngines();
    offerBoth(list);
    QVERIFY(list->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(search->urlForInput(QStringLiteral("forest fires")),
             QStringLiteral("https://www.seek.example/look/forest%20fires/"));
    QVERIFY(!search->isAddress(QStringLiteral("forest fires")));
    QCOMPARE(search->urlForInput(QStringLiteral("example.org/page")),
             QStringLiteral("https://example.org/page"));
    QVERIFY(search->isAddress(QStringLiteral("example.org/page")));

    QVERIFY(list->addFoundEngine(
        QLatin1String(FindHref),
        description(QStringLiteral("Find"), QStringLiteral("https://find.example/{searchTerms}"))));
    QCOMPARE(search->urlForInput(QStringLiteral("a")), QStringLiteral("https://find.example/a"));
    QVERIFY(!list->isSearchUrl(QStringLiteral("https://find.example/anything")));
}

QTEST_GUILESS_MAIN(tst_searchengines)
#include "tst_searchengines.moc"
