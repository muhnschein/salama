// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "settings/SearchSettings.h"
#include "settings/SettingsSections.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::SearchSettings;
using Sections = Salama::SettingsSections;

namespace {

const char *const FindHref = "https://find.example/opensearch.xml";
const char *const SeekHref = "https://seek.example/description.xml";

// What a site's description says: its name, and a page of results with the words in the
// query, or in the path.
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

// Offers the two sites' searches, as pages of theirs would.
void offerBoth(SearchSettings *search)
{
    QVERIFY(search->offerEngine(QStringLiteral("Find"), QLatin1String(FindHref),
                                QStringLiteral("find.example")));
    QVERIFY(search->offerEngine(QStringLiteral("Seek"), QLatin1String(SeekHref),
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
    void storedGarbageIsPassedOver();
    void searchPagesOfAddedEngines();
    void typedTextWithAnAddedEngine();
};

void tst_searchengines::builtInOnly()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    QCOMPARE(search->engineNames(), QStringList({QStringLiteral("Qwant"), QStringLiteral("Ecosia"),
                                                 QStringLiteral("Startpage")}));
    QCOMPARE(search->engineKeys(), QStringList({QStringLiteral("qwant"), QStringLiteral("ecosia"),
                                                QStringLiteral("startpage")}));
    QCOMPARE(search->engineHosts(), QStringList({QString(), QString(), QString()}));
    QCOMPARE(search->addedCount(), 0);
    QVERIFY(search->foundEngines().isEmpty());
}

void tst_searchengines::offersAreKept()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    QSignalSpy found(search, &SearchSettings::foundChanged);
    QSignalSpy engines(search, &SearchSettings::enginesChanged);

    offerBoth(search);
    QCOMPARE(found.count(), 2);
    // An offer is not an engine: nothing in the list or in the choice moved.
    QCOMPARE(engines.count(), 0);
    QCOMPARE(search->engineNames().count(), 3);

    const QVariantList offers = search->foundEngines();
    QCOMPARE(titlesOf(offers), QStringList({QStringLiteral("Find"), QStringLiteral("Seek")}));
    QCOMPARE(offers.first().toMap().value(QStringLiteral("href")).toString(),
             QLatin1String(FindHref));
    QCOMPARE(offers.first().toMap().value(QStringLiteral("host")).toString(),
             QStringLiteral("find.example"));
}

// The page says it on every visit, and a site may say it twice; what is on offer, or
// in the list, or named like what is, is not kept again. The host is the page's, or the
// description's when the page's is not given.
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
    SearchSettings *search = settings.search();
    QVERIFY(search->offerEngine(QStringLiteral("Added"), QStringLiteral("https://added.example/o"),
                                QString()));
    QVERIFY(search->addFoundEngine(
        QStringLiteral("https://added.example/o"),
        description(QStringLiteral("Added"),
                    QStringLiteral("https://added.example/?q={searchTerms}"))));
    QVERIFY(search->offerEngine(QStringLiteral("Find"), QLatin1String(FindHref), QString()));

    QSignalSpy found(search, &SearchSettings::foundChanged);
    QCOMPARE(search->offerEngine(title, href, QString()), kept);
    QCOMPARE(found.count(), kept ? 1 : 0);
    QCOMPARE(search->foundEngines().count(), kept ? 2 : 1);
}

void tst_searchengines::offersArePersisted()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        QVERIFY(settings.search()->offerEngine(QStringLiteral("Find, \"Seek\" & co"),
                                               QLatin1String(FindHref), QString()));
        QVERIFY(settings.search()->offerEngine(QStringLiteral("Seek"), QLatin1String(SeekHref),
                                               QStringLiteral("seek.example")));
    }
    Sections again(path);
    const QVariantList offers = again.search()->foundEngines();
    QCOMPARE(titlesOf(offers),
             QStringList({QStringLiteral("Find, \"Seek\" & co"), QStringLiteral("Seek")}));
    // No host given: the description's, without "www.".
    QCOMPARE(offers.first().toMap().value(QStringLiteral("host")).toString(),
             QStringLiteral("find.example"));
    QCOMPARE(offers.last().toMap().value(QStringLiteral("host")).toString(),
             QStringLiteral("seek.example"));
    // And what was kept is not kept again.
    QVERIFY(!again.search()->offerEngine(QStringLiteral("seek"),
                                         QStringLiteral("https://x.example/"), QString()));
}

void tst_searchengines::forgettingAnOffer()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    Sections settings(path);
    SearchSettings *search = settings.search();
    offerBoth(search);
    QSignalSpy found(search, &SearchSettings::foundChanged);

    search->forgetFoundEngine(QStringLiteral("https://nothing.example/"));
    QCOMPARE(found.count(), 0);
    search->forgetFoundEngine(QLatin1String(FindHref));
    QCOMPARE(found.count(), 1);
    QCOMPARE(titlesOf(search->foundEngines()), QStringList{QStringLiteral("Seek")});
    QCOMPARE(titlesOf(Sections(path).search()->foundEngines()),
             QStringList{QStringLiteral("Seek")});

    // Forgotten, it may be offered again: the site may still have it.
    QVERIFY(search->offerEngine(QStringLiteral("Find"), QLatin1String(FindHref), QString()));
    search->forgetFoundEngine(QLatin1String(FindHref));
    search->forgetFoundEngine(QLatin1String(SeekHref));
    QVERIFY(search->foundEngines().isEmpty());
    QVERIFY(Sections(path).search()->foundEngines().isEmpty());
}

void tst_searchengines::addingAnEngine()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    offerBoth(search);
    QSignalSpy engines(search, &SearchSettings::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);
    QSignalSpy found(search, &SearchSettings::foundChanged);

    QVERIFY(search->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QCOMPARE(engines.count(), 1);
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(found.count(), 1);
    QCOMPARE(search->engineNames().last(), QStringLiteral("Find"));
    QCOMPARE(search->engineNames().count(), 4);
    QCOMPARE(search->engineHosts(),
             QStringList({QString(), QString(), QString(), QStringLiteral("find.example")}));
    QCOMPARE(search->addedCount(), 1);
    // Taken up: no longer an offer, and the engine in use.
    QCOMPARE(titlesOf(search->foundEngines()), QStringList{QStringLiteral("Seek")});
    QCOMPARE(search->engineIndex(), 3);
    QCOMPARE(search->engine(), search->engineKeys().last());
    QCOMPARE(search->engineNames().at(search->engineIndex()), QStringLiteral("Find"));
    QCOMPARE(search->searchUrl(QStringLiteral("sailfish os")),
             QStringLiteral("https://find.example/results?query=sailfish%20os"));

    // The name is the description's, not the page's title.
    QVERIFY(search->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(search->engineNames().last(), QStringLiteral("Seek"));
    QCOMPARE(search->engineIndex(), 4);
    QVERIFY(search->foundEngines().isEmpty());
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
    QVERIFY(search->offerEngine(QStringLiteral("Find"), QLatin1String(FindHref), QString()));
    QSignalSpy engines(search, &SearchSettings::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);
    QSignalSpy found(search, &SearchSettings::foundChanged);

    QVERIFY(!search->addFoundEngine(href, text));
    // Nothing moved: the offer stays, and the engine in use is as it was.
    QCOMPARE(engines.count() + chosen.count() + found.count(), 0);
    QCOMPARE(search->engineNames().count(), 3);
    QCOMPARE(search->addedCount(), 0);
    QCOMPARE(search->engine(), SearchSettings::defaultEngine());
    QCOMPARE(titlesOf(search->foundEngines()), QStringList{QStringLiteral("Find")});
}

void tst_searchengines::addedEnginesArePersisted()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Sections settings(path);
        offerBoth(settings.search());
        QVERIFY(settings.search()->addFoundEngine(QLatin1String(FindHref), findDescription()));
    }
    Sections again(path);
    SearchSettings *search = again.search();
    QCOMPARE(search->engineNames().last(), QStringLiteral("Find"));
    QCOMPARE(search->engineHosts().last(), QStringLiteral("find.example"));
    // Still the engine in use, and still the one that searches.
    QCOMPARE(search->engineIndex(), 3);
    QCOMPARE(search->searchUrl(QStringLiteral("a b")),
             QStringLiteral("https://find.example/results?query=a%20b"));
    QCOMPARE(titlesOf(search->foundEngines()), QStringList{QStringLiteral("Seek")});
    QCOMPARE(search->addedCount(), 1);
}

void tst_searchengines::addedEnginesUseTheirTemplates()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    offerBoth(search);
    QVERIFY(search->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(search->searchUrl(QStringLiteral(" 100% a&b ")),
             QStringLiteral("https://www.seek.example/look/100%25%20a%26b/"));
    // A built-in engine is as it was.
    search->setEngineIndex(1);
    QCOMPARE(search->searchUrl(QStringLiteral("a")),
             QStringLiteral("https://www.ecosia.org/search?q=a"));
    search->setEngine(search->engineKeys().last());
    QCOMPARE(search->searchUrl(QStringLiteral("a")),
             QStringLiteral("https://www.seek.example/look/a/"));
}

void tst_searchengines::keysAreUnique()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    const QStringList names{QStringLiteral("A B"), QStringLiteral("A-B"), QStringLiteral("A.B"),
                            QStringLiteral("???")};
    for (int i = 0; i < names.count(); ++i) {
        const QString href = QStringLiteral("https://site%1.example/o.xml").arg(i);
        QVERIFY(search->offerEngine(names.at(i), href, QString()));
        QVERIFY(search->addFoundEngine(
            href, description(names.at(i),
                              QStringLiteral("https://site%1.example/?q={searchTerms}").arg(i))));
    }
    QStringList keys = search->engineKeys();
    QCOMPARE(keys.count(), 7);
    QCOMPARE(keys.removeDuplicates(), 0);
    // Each is the one chosen when it is chosen, whatever the others are named.
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
    offerBoth(search);
    QVERIFY(search->addFoundEngine(QLatin1String(FindHref), findDescription()));
    const QString key = search->engine();
    QSignalSpy engines(search, &SearchSettings::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);

    search->removeAddedEngine(key);
    QCOMPARE(engines.count(), 1);
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(search->engineNames().count(), 3);
    QCOMPARE(search->addedCount(), 0);
    // The first built-in engine takes its place, here and in the file.
    QCOMPARE(search->engine(), SearchSettings::defaultEngine());
    QCOMPARE(search->engineIndex(), 0);
    QCOMPARE(QSettings(path, QSettings::IniFormat).value(QStringLiteral("searchEngine")).toString(),
             SearchSettings::defaultEngine());
    QCOMPARE(Sections(path).search()->engineNames().count(), 3);
    QCOMPARE(search->searchUrl(QStringLiteral("a")), QStringLiteral("https://www.qwant.com/?q=a"));
}

void tst_searchengines::removingAnotherEngine()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    offerBoth(search);
    QVERIFY(search->addFoundEngine(QLatin1String(FindHref), findDescription()));
    const QString find = search->engine();
    QVERIFY(search->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    const QString seek = search->engine();

    // The one in use is Seek; removing Find, which comes before it, moves where it is in
    // the list and nothing else.
    search->removeAddedEngine(find);
    QCOMPARE(search->engine(), seek);
    QCOMPARE(search->engineNames().last(), QStringLiteral("Seek"));
    QCOMPARE(search->engineIndex(), 3);
    QCOMPARE(search->addedCount(), 1);

    search->setEngineIndex(2);
    search->removeAddedEngine(seek);
    QCOMPARE(search->engineKeys().count(), 3);
    QCOMPARE(search->engine(), QStringLiteral("startpage"));
}

void tst_searchengines::removingWhatCannotBeRemoved()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    QSignalSpy engines(search, &SearchSettings::enginesChanged);
    search->removeAddedEngine(QStringLiteral("qwant"));
    search->removeAddedEngine(QStringLiteral("added-nothing"));
    search->removeAddedEngine(QString());
    QCOMPARE(engines.count(), 0);
    QCOMPARE(search->engineNames().count(), 3);
}

void tst_searchengines::resetFallsBack()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    Sections settings(path);
    SearchSettings *search = settings.search();
    offerBoth(search);
    QVERIFY(search->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QVERIFY(search->offerEngine(QStringLiteral("Third"), QStringLiteral("https://third.example/o"),
                                QString()));
    QCOMPARE(search->engineIndex(), 3);
    QSignalSpy engines(search, &SearchSettings::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);
    QSignalSpy found(search, &SearchSettings::foundChanged);

    search->removeAddedEngines();
    QCOMPARE(engines.count(), 1);
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(found.count(), 1);
    QCOMPARE(search->engineNames().count(), 3);
    QVERIFY(search->foundEngines().isEmpty());
    QCOMPARE(search->addedCount(), 0);
    QCOMPARE(search->engine(), SearchSettings::defaultEngine());
    QCOMPARE(search->engineIndex(), 0);
    // Nothing of it is left in the file either.
    Sections again(path);
    QCOMPARE(again.search()->engineNames().count(), 3);
    QVERIFY(again.search()->foundEngines().isEmpty());
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
    offerBoth(search);
    QVERIFY(search->addFoundEngine(QLatin1String(FindHref), findDescription()));
    search->setEngine(QStringLiteral("startpage"));
    QSignalSpy chosen(search, &SearchSettings::engineChanged);

    search->removeAddedEngines();
    QCOMPARE(search->engine(), QStringLiteral("startpage"));
    QCOMPARE(search->engineNames().count(), 3);
    QVERIFY(search->foundEngines().isEmpty());
    // The list changed, so what is where in it is said again, and the choice is as it was.
    QCOMPARE(search->engineIndex(), 2);
}

void tst_searchengines::resetOfNothingSaysNothing()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    QSignalSpy engines(search, &SearchSettings::enginesChanged);
    QSignalSpy chosen(search, &SearchSettings::engineChanged);
    QSignalSpy found(search, &SearchSettings::foundChanged);
    search->removeAddedEngines();
    QCOMPARE(engines.count() + chosen.count() + found.count(), 0);

    // Offers alone: forgotten, and the engines are not said to have changed.
    offerBoth(search);
    found.clear();
    search->removeAddedEngines();
    QCOMPARE(found.count(), 1);
    QCOMPARE(engines.count() + chosen.count(), 0);
}

void tst_searchengines::indexFollowsTheList()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    offerBoth(search);
    QVERIFY(search->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QVERIFY(search->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(search->engineIndex(), 4);
    search->setEngineIndex(5);
    QCOMPARE(search->engineIndex(), 4);
    search->setEngineIndex(3);
    QCOMPARE(search->engineNames().at(search->engineIndex()), QStringLiteral("Find"));
    search->setEngineIndex(0);
    QCOMPARE(search->engine(), QStringLiteral("qwant"));
}

// The file is one a user can edit: records that are no engine, offers with no address,
// and text that is no JSON at all are passed over, and what is sound is kept.
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
    QCOMPARE(search->engineNames().mid(3), QStringList{QStringLiteral("Ok")});
    QCOMPARE(search->engine(), QStringLiteral("added-ok"));
    QCOMPARE(titlesOf(search->foundEngines()), QStringList{QStringLiteral("Fine")});

    {
        QSettings file(path, QSettings::IniFormat);
        file.setValue(QStringLiteral("searchEnginesAdded"), QStringLiteral("not json {"));
        file.setValue(QStringLiteral("searchEnginesFound"), QStringLiteral("{\"title\":\"x\"}"));
    }
    Sections broken(path);
    QCOMPARE(broken.search()->engineNames().count(), 3);
    QVERIFY(broken.search()->foundEngines().isEmpty());
    // The engine it names is gone, so the first built-in one is the engine.
    QCOMPARE(broken.search()->engine(), SearchSettings::defaultEngine());
}

// Pages of results of an added engine are searches, as the built-in ones are, however
// the engine put its words in the address; the start page reads the history by this
// (src/startpage/StartPage.cpp).
void tst_searchengines::searchPagesOfAddedEngines()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    const QString find = QStringLiteral("https://find.example/results?query=forest");
    const QString seek = QStringLiteral("https://seek.example/look/forest/");
    QVERIFY(!search->isSearchUrl(find));
    QVERIFY(!search->isSearchUrl(seek));

    offerBoth(search);
    QVERIFY(search->addFoundEngine(QLatin1String(FindHref), findDescription()));
    QVERIFY(search->isSearchUrl(find));
    // With a parameter of its own ahead of the words, and with or without "www.".
    QVERIFY(search->isSearchUrl(QStringLiteral("https://www.find.example/results?x=1&query=a")));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://find.example/results")));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://find.example/other?query=forest")));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://find.example/")));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://elsewhere.example/results?query=a")));
    QVERIFY(!search->isSearchUrl(seek));

    // Words in the path: what stands before them and after them is what makes the page
    // a search, and the front page of the site is not.
    QVERIFY(search->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QVERIFY(search->isSearchUrl(seek));
    QVERIFY(search->isSearchUrl(QStringLiteral("https://www.seek.example/look/two%20words/")));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://seek.example/look/")));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://seek.example/look/forest")));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://seek.example/")));
    QVERIFY(search->isSearchUrl(find));

    // The built-in ones are as they were.
    QVERIFY(search->isSearchUrl(QStringLiteral("https://www.qwant.com/?q=a")));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://www.qwant.com/")));

    // And once removed, they are sites again.
    search->removeAddedEngines();
    QVERIFY(!search->isSearchUrl(find));
    QVERIFY(!search->isSearchUrl(seek));
    QVERIFY(search->isSearchUrl(QStringLiteral("https://www.qwant.com/?q=a")));
}

// What is typed is searched for with the engine in use, added or not, and what is an
// address is still one. An engine that would take in every page of its site -- the
// words as the whole path -- is searched with all the same, but is not read for its
// results: the start page would take the whole site for a search.
void tst_searchengines::typedTextWithAnAddedEngine()
{
    QTemporaryDir dir;
    Sections settings(dir.path() + QStringLiteral("/salama.conf"));
    SearchSettings *search = settings.search();
    offerBoth(search);
    QVERIFY(search->addFoundEngine(QLatin1String(SeekHref), seekDescription()));
    QCOMPARE(search->urlForInput(QStringLiteral("forest fires")),
             QStringLiteral("https://www.seek.example/look/forest%20fires/"));
    QVERIFY(!search->isAddress(QStringLiteral("forest fires")));
    QCOMPARE(search->urlForInput(QStringLiteral("example.org/page")),
             QStringLiteral("https://example.org/page"));
    QVERIFY(search->isAddress(QStringLiteral("example.org/page")));

    QVERIFY(search->addFoundEngine(
        QLatin1String(FindHref),
        description(QStringLiteral("Find"), QStringLiteral("https://find.example/{searchTerms}"))));
    QCOMPARE(search->urlForInput(QStringLiteral("a")), QStringLiteral("https://find.example/a"));
    QVERIFY(!search->isSearchUrl(QStringLiteral("https://find.example/anything")));
}

QTEST_GUILESS_MAIN(tst_searchengines)
#include "tst_searchengines.moc"
