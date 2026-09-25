// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "search/SearchWords.h"

#include <QtTest>

using Salama::SearchWords;

class tst_searchwords : public QObject
{
    Q_OBJECT

private slots:
    void splitsOnWhitespace_data();
    void splitsOnWhitespace();
    void everyWordInSomeField();
    void unicodeCase();
    void prefixes();
    void prefixesAWord_data();
    void prefixesAWord();
    void marked_data();
    void marked();
};

void tst_searchwords::splitsOnWhitespace_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QStringList>("words");

    QTest::newRow("empty") << QString() << QStringList();
    QTest::newRow("blank") << QStringLiteral(" \t \n") << QStringList();
    QTest::newRow("one") << QStringLiteral("sailfish") << QStringList{QStringLiteral("sailfish")};
    QTest::newRow("trimmed and collapsed")
        << QStringLiteral("  news \t  helsinki ")
        << QStringList{QStringLiteral("news"), QStringLiteral("helsinki")};
    // A no-break space is whitespace too, as a phone's keyboard may type one.
    QTest::newRow("no-break space")
        << QStringLiteral("jolla phone")
        << QStringList{QStringLiteral("jolla"), QStringLiteral("phone")};
    // An address is one word: nothing but whitespace divides.
    QTest::newRow("address") << QStringLiteral("example.org/a?b=c")
                             << QStringList{QStringLiteral("example.org/a?b=c")};
}

void tst_searchwords::splitsOnWhitespace()
{
    QFETCH(QString, text);
    QFETCH(QStringList, words);
    const SearchWords search(text);
    QCOMPARE(search.words(), words);
    QCOMPARE(search.isEmpty(), words.isEmpty());
}

void tst_searchwords::everyWordInSomeField()
{
    const QStringList page{QStringLiteral("Helsinki news"),
                           QStringLiteral("https://yle.fi/uutiset")};

    // Each word may be found in a field of its own, and in any order.
    QVERIFY(SearchWords(QStringLiteral("news helsinki")).matches(page));
    QVERIFY(SearchWords(QStringLiteral("yle news")).matches(page));
    QVERIFY(SearchWords(QStringLiteral("NEWS")).matches(page));
    QVERIFY(SearchWords(QStringLiteral("ki ne")).matches(page));
    // Every word, not any: one missing word is no match.
    QVERIFY(!SearchWords(QStringLiteral("news tampere")).matches(page));
    QVERIFY(!SearchWords(QStringLiteral("weather")).matches(page));
    // A word is not a phrase: across the space between two words of a field it is
    // not found.
    QVERIFY(!SearchWords(QStringLiteral("inews")).matches(page));

    // Nothing typed hides nothing, and no fields hold nothing.
    QVERIFY(SearchWords().matches(page));
    QVERIFY(SearchWords().matches({}));
    QVERIFY(!SearchWords(QStringLiteral("news")).matches({}));
    QVERIFY(!SearchWords(QStringLiteral("news")).matches({QString()}));
}

// Unicode's case folding, not ASCII's: what SQLite's LIKE would have missed.
void tst_searchwords::unicodeCase()
{
    QVERIFY(SearchWords(QStringLiteral("äly")).matches({QStringLiteral("Älypuhelin")}));
    QVERIFY(SearchWords(QStringLiteral("ÄLY")).matches({QStringLiteral("älypuhelin")}));
    QVERIFY(SearchWords(QStringLiteral("ÖLJY")).matches({QStringLiteral("Öljynvaihto")}));
    QVERIFY(SearchWords(QStringLiteral("москва")).matches({QStringLiteral("МОСКВА")}));
    QVERIFY(SearchWords(QStringLiteral("ΑΘΉΝΑ")).matches({QStringLiteral("Αθήνα")}));
    // A letter is not its unaccented neighbour.
    QVERIFY(!SearchWords(QStringLiteral("aly")).matches({QStringLiteral("Älypuhelin")}));
}

void tst_searchwords::prefixes()
{
    const SearchWords search(QStringLiteral("Wiki sailfish"));
    // By the first word only, whatever its case.
    QVERIFY(search.prefixes(QStringLiteral("wikipedia.org")));
    QVERIFY(!search.prefixes(QStringLiteral("en.wikipedia.org")));
    QVERIFY(!search.prefixes(QStringLiteral("sailfish.org")));
    QVERIFY(!SearchWords().prefixes(QStringLiteral("wikipedia.org")));
    QVERIFY(SearchWords(QStringLiteral("äly")).prefixes(QStringLiteral("Älypuhelin")));
}

void tst_searchwords::prefixesAWord_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("expected");

    QTest::newRow("first word") << QStringLiteral("sail") << QStringLiteral("Sailfish OS") << true;
    QTest::newRow("later word") << QStringLiteral("os") << QStringLiteral("Sailfish OS") << true;
    QTest::newRow("after punctuation")
        << QStringLiteral("wiki") << QStringLiteral("Sailfish (Wikipedia)") << true;
    QTest::newRow("after a dash") << QStringLiteral("news") << QStringLiteral("Yle-news") << true;
    QTest::newRow("inside a word")
        << QStringLiteral("fish") << QStringLiteral("Sailfish OS") << false;
    // Found inside a word first and at the start of one later: the later counts.
    QTest::newRow("second occurrence")
        << QStringLiteral("fish") << QStringLiteral("Sailfish fishing") << true;
    QTest::newRow("unicode") << QStringLiteral("äly") << QStringLiteral("Uusi Älypuhelin") << true;
    QTest::newRow("after a digit") << QStringLiteral("x") << QStringLiteral("3x speed") << false;
    QTest::newRow("absent") << QStringLiteral("jolla") << QStringLiteral("Sailfish OS") << false;
    QTest::newRow("nothing typed") << QString() << QStringLiteral("Sailfish OS") << false;
    QTest::newRow("empty text") << QStringLiteral("sail") << QString() << false;
}

void tst_searchwords::prefixesAWord()
{
    QFETCH(QString, query);
    QFETCH(QString, text);
    QFETCH(bool, expected);
    QCOMPARE(SearchWords(query).prefixesAWordOf(text), expected);
}

// Every place a word typed appears, in bold, whatever its case; places that touch or
// overlap are one; everything else is escaped, a page's markup included.
void tst_searchwords::marked_data()
{
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("styled");

    QTest::newRow("nothing typed")
        << QString() << QStringLiteral("A & B") << QStringLiteral("A &amp; B");
    QTest::newRow("no match") << QStringLiteral("x") << QStringLiteral("Forest")
                              << QStringLiteral("Forest");
    QTest::newRow("case") << QStringLiteral("FOR") << QStringLiteral("Forest for all")
                          << QStringLiteral("<b>For</b>est <b>for</b> all");
    QTest::newRow("two words") << QStringLiteral("news helsinki") << QStringLiteral("Helsinki news")
                               << QStringLiteral("<b>Helsinki</b> <b>news</b>");
    QTest::newRow("overlapping") << QStringLiteral("ab bc") << QStringLiteral("xabcx")
                                 << QStringLiteral("x<b>abc</b>x");
    QTest::newRow("touching") << QStringLiteral("ab cd") << QStringLiteral("abcd")
                              << QStringLiteral("<b>abcd</b>");
    QTest::newRow("unicode") << QStringLiteral("äly") << QStringLiteral("ÄLYKELLO")
                             << QStringLiteral("<b>ÄLY</b>KELLO");
    QTest::newRow("markup") << QStringLiteral("b") << QStringLiteral("<b>x</b> & \"b\"")
                            << QStringLiteral(
                                   "&lt;<b>b</b>&gt;x&lt;/<b>b</b>&gt; &amp; &quot;<b>b</b>&quot;");
    QTest::newRow("markup typed") << QStringLiteral("<img") << QStringLiteral("a <img src=x>")
                                  << QStringLiteral("a <b>&lt;img</b> src=x&gt;");
}

void tst_searchwords::marked()
{
    QFETCH(QString, typed);
    QFETCH(QString, text);
    QFETCH(QString, styled);
    QCOMPARE(SearchWords(typed).marked(text), styled);
}

QTEST_GUILESS_MAIN(tst_searchwords)
#include "tst_searchwords.moc"
