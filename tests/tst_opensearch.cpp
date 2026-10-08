// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "search/OpenSearch.h"

#include <QtTest>

using Salama::OpenSearch;
using Salama::OpenSearchEngine;

namespace {

QString description(const QString &body, const QString &name = QStringLiteral("Find"))
{
    return QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                          "<OpenSearchDescription xmlns=\"http://a9.com/-/spec/opensearch/1.1/\">"
                          "<ShortName>%1</ShortName><Description>Search Find</Description>%2"
                          "</OpenSearchDescription>")
        .arg(name, body);
}

QString url(const QString &attributes)
{
    return QStringLiteral("<Url %1/>").arg(attributes);
}

} // namespace

class tst_opensearch : public QObject
{
    Q_OBJECT

private slots:
    void parsesTheResultsPage();
    void refusesWhatIsNotUsable_data();
    void refusesWhatIsNotUsable();
    void readsTheOtherParameters_data();
    void readsTheOtherParameters();
    void addsParams();
    void takesTheFirstResultsPage();
    void onlyTheFirstShortNameIsTheName();
    void namelessIsStillAnEngine();
    void refusesALargeDocument();
    void fill();
    void isTemplate();
};

void tst_opensearch::parsesTheResultsPage()
{
    const OpenSearchEngine engine = OpenSearch::parse(description(
        url(QStringLiteral("type=\"text/html\" method=\"GET\" "
                           "template=\"https://find.example/search?q={searchTerms}\""))));
    QVERIFY(engine.isValid());
    QCOMPARE(engine.name, QStringLiteral("Find"));
    QCOMPARE(engine.urlTemplate, QStringLiteral("https://find.example/search?q={searchTerms}"));
}

void tst_opensearch::refusesWhatIsNotUsable_data()
{
    QTest::addColumn<QString>("xml");

    const QString good = QStringLiteral("template=\"https://find.example/s?q={searchTerms}\"");
    QTest::newRow("post only") << description(
        url(QStringLiteral("type=\"text/html\" method=\"POST\" ") + good));
    QTest::newRow("post, lower case")
        << description(url(QStringLiteral("type=\"text/html\" method=\"post\" ") + good));
    QTest::newRow("no text/html url")
        << description(url(QStringLiteral("type=\"application/x-suggestions+json\" ") + good) +
                       url(QStringLiteral("type=\"application/rss+xml\" ") + good));
    QTest::newRow("no type") << description(url(good));
    QTest::newRow("suggestions") << description(
        url(QStringLiteral("type=\"text/html\" rel=\"suggestions\" ") + good));
    QTest::newRow("ftp") << description(url(
        QStringLiteral("type=\"text/html\" template=\"ftp://find.example/s?q={searchTerms}\"")));
    QTest::newRow("javascript") << description(
        url(QStringLiteral("type=\"text/html\" template=\"javascript:alert('{searchTerms}')\"")));
    QTest::newRow("relative") << description(
        url(QStringLiteral("type=\"text/html\" template=\"/s?q={searchTerms}\"")));
    QTest::newRow("no scheme") << description(
        url(QStringLiteral("type=\"text/html\" template=\"//find.example/s?q={searchTerms}\"")));
    QTest::newRow("no search terms") << description(
        url(QStringLiteral("type=\"text/html\" template=\"https://find.example/s?q={count}\"")));
    QTest::newRow("no template") << description(url(QStringLiteral("type=\"text/html\"")));
    QTest::newRow("search terms in the host") << description(
        url(QStringLiteral("type=\"text/html\" template=\"https://{searchTerms}.example/\"")));
    QTest::newRow("search terms in the scheme") << description(
        url(QStringLiteral("type=\"text/html\" template=\"{searchTerms}://find.example/\"")));
    QTest::newRow("no host") << description(
        url(QStringLiteral("type=\"text/html\" template=\"https:///s?q={searchTerms}\"")));
    QTest::newRow("no urls") << description(QString());
    QTest::newRow("truncated")
        << description(url(QStringLiteral("type=\"text/html\" ") + good)).left(200);
    QTest::newRow("good, then broken")
        << description(url(QStringLiteral("type=\"text/html\" ") + good) + QStringLiteral("<Url"));
    QTest::newRow("html page") << QStringLiteral("<html><body><p>Not found<br></body></html>");
    QTest::newRow("garbage") << QStringLiteral("\x01\x02 not xml at all {searchTerms}");
    QTest::newRow("json") << QStringLiteral(
        "{\"template\": \"https://find.example/{searchTerms}\"}");
    QTest::newRow("empty") << QString();
}

void tst_opensearch::refusesWhatIsNotUsable()
{
    QFETCH(QString, xml);
    const OpenSearchEngine engine = OpenSearch::parse(xml);
    QVERIFY(!engine.isValid());
    QVERIFY(engine.urlTemplate.isEmpty());
    QVERIFY(engine.name.isEmpty());
}

// Other OpenSearch params: optional dropped, rest empty (every description allows).
void tst_opensearch::readsTheOtherParameters_data()
{
    QTest::addColumn<QString>("given");
    QTest::addColumn<QString>("expected");

    QTest::newRow("optional count")
        << QStringLiteral("https://find.example/s?q={searchTerms}&n={count?}")
        << QStringLiteral("https://find.example/s?q={searchTerms}&n=");
    QTest::newRow("optional search terms")
        << QStringLiteral("https://find.example/s?q={searchTerms?}")
        << QStringLiteral("https://find.example/s?q={searchTerms}");
    QTest::newRow("language, encodings")
        << QStringLiteral("https://find.example/{language}/s?ie={inputEncoding}&q={searchTerms}"
                          "&oe={outputEncoding}&p={startPage?}")
        << QStringLiteral("https://find.example//s?ie=&q={searchTerms}&oe=&p=");
    QTest::newRow("extension parameter")
        << QStringLiteral("https://find.example/s?q={searchTerms}&c={moz:distributionID}")
        << QStringLiteral("https://find.example/s?q={searchTerms}&c=");
    QTest::newRow("in the path") << QStringLiteral("https://find.example/search/{searchTerms}.html")
                                 << QStringLiteral(
                                        "https://find.example/search/{searchTerms}.html");
    QTest::newRow("twice")
        << QStringLiteral("https://find.example/s?q={searchTerms}&alt={searchTerms}")
        << QStringLiteral("https://find.example/s?q={searchTerms}&alt={searchTerms}");
    QTest::newRow("surrounding space")
        << QStringLiteral("  https://find.example/s?q={searchTerms}\n")
        << QStringLiteral("https://find.example/s?q={searchTerms}");
    QTest::newRow("http, a port") << QStringLiteral("http://find.example:8080/s?q={searchTerms}")
                                  << QStringLiteral("http://find.example:8080/s?q={searchTerms}");
}

void tst_opensearch::readsTheOtherParameters()
{
    QFETCH(QString, given);
    QFETCH(QString, expected);
    const OpenSearchEngine engine = OpenSearch::parse(description(
        url(QStringLiteral("type=\"text/html\" template=\"%1\"").arg(given.toHtmlEscaped()))));
    QVERIFY(engine.isValid());
    QCOMPARE(engine.urlTemplate, expected);
}

// <Param> children kept apart from url, as Firefox's own.
void tst_opensearch::addsParams()
{
    OpenSearchEngine engine = OpenSearch::parse(description(QStringLiteral(
        "<Url type=\"text/html\" template=\"https://find.example/s\">"
        "<Param name=\"q\" value=\"{searchTerms}\"/><Param name=\"client\" value=\"salama app\"/>"
        "<Param name=\"\" value=\"nameless\"/>"
        "</Url>")));
    QVERIFY(engine.isValid());
    QCOMPARE(engine.urlTemplate,
             QStringLiteral("https://find.example/s?q={searchTerms}&client=salama%20app"));

    engine = OpenSearch::parse(description(
        QStringLiteral("<Url type=\"text/html\" template=\"https://find.example/s?hl=fi\">"
                       "<Param name=\"q\" value=\"{searchTerms}\"/></Url>")));
    QCOMPARE(engine.urlTemplate, QStringLiteral("https://find.example/s?hl=fi&q={searchTerms}"));
}

void tst_opensearch::takesTheFirstResultsPage()
{
    const OpenSearchEngine engine = OpenSearch::parse(description(
        url(QStringLiteral("type=\"application/x-suggestions+json\" "
                           "template=\"https://find.example/suggest?q={searchTerms}\"")) +
        url(QStringLiteral("type=\"text/html\" method=\"post\" "
                           "template=\"https://find.example/post\"")) +
        url(QStringLiteral("type=\"text/html\" rel=\"results\" "
                           "template=\"https://find.example/first?q={searchTerms}\"")) +
        url(QStringLiteral("type=\"text/html\" template=\"https://find.example/second?q="
                           "{searchTerms}\""))));
    QCOMPARE(engine.urlTemplate, QStringLiteral("https://find.example/first?q={searchTerms}"));
}

void tst_opensearch::onlyTheFirstShortNameIsTheName()
{
    const OpenSearchEngine engine = OpenSearch::parse(
        description(QStringLiteral("<ShortName>Other</ShortName>") +
                        url(QStringLiteral("type=\"text/html\" template=\"https://find.example/?q="
                                           "{searchTerms}\"")),
                    QStringLiteral("  Find &amp; Seek \n")));
    QCOMPARE(engine.name, QStringLiteral("Find & Seek"));
}

void tst_opensearch::namelessIsStillAnEngine()
{
    const OpenSearchEngine engine = OpenSearch::parse(QStringLiteral(
        "<OpenSearchDescription><Url type=\"text/html\" "
        "template=\"https://find.example/?q={searchTerms}\"/></OpenSearchDescription>"));
    QVERIFY(engine.isValid());
    QVERIFY(engine.name.isEmpty());
}

void tst_opensearch::refusesALargeDocument()
{
    const QString padding(70 * 1024, QLatin1Char(' '));
    const QString good = description(url(
        QStringLiteral("type=\"text/html\" template=\"https://find.example/?q={searchTerms}\"")));
    QVERIFY(OpenSearch::parse(good).isValid());
    QVERIFY(!OpenSearch::parse(good + padding).isValid());
}

void tst_opensearch::fill()
{
    const QString address = QStringLiteral("https://find.example/s?q={searchTerms}&alt=1");
    QCOMPARE(OpenSearch::fill(address, QStringLiteral(" sailfish os ")),
             QStringLiteral("https://find.example/s?q=sailfish%20os&alt=1"));
    QCOMPARE(OpenSearch::fill(address, QStringLiteral("a&b=c#d")),
             QStringLiteral("https://find.example/s?q=a%26b%3Dc%23d&alt=1"));
    QCOMPARE(OpenSearch::fill(address, QStringLiteral("100%")),
             QStringLiteral("https://find.example/s?q=100%25&alt=1"));
    // Search words not re-read as template.
    QCOMPARE(OpenSearch::fill(address, QStringLiteral("{searchTerms}")),
             QStringLiteral("https://find.example/s?q=%7BsearchTerms%7D&alt=1"));
    // Template's own % not treated as arg list.
    QCOMPARE(OpenSearch::fill(QStringLiteral("https://find.example/%20/%1?q={searchTerms}"),
                              QStringLiteral("x")),
             QStringLiteral("https://find.example/%20/%1?q=x"));
}

void tst_opensearch::isTemplate()
{
    QVERIFY(OpenSearch::isTemplate(QStringLiteral("https://find.example/?q={searchTerms}")));
    QVERIFY(!OpenSearch::isTemplate(QStringLiteral("https://find.example/?q=%1")));
    QVERIFY(!OpenSearch::isTemplate(QStringLiteral("file:///tmp/{searchTerms}")));
    QVERIFY(!OpenSearch::isTemplate(QStringLiteral("https://{searchTerms}/")));
    QVERIFY(!OpenSearch::isTemplate(QString()));
}

QTEST_GUILESS_MAIN(tst_opensearch)
#include "tst_opensearch.moc"
