// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "engine/TabHistory.h"
#include "history/HistoryModel.h"
#include "storage/Storage.h"

#include <QJSEngine>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

using Salama::HistoryModel;
using Salama::Storage;
using Salama::TabHistory;

namespace {

QVariantMap backEntry(const QVariant &index, const QString &address,
                      const QString &title = QString())
{
    return {{QStringLiteral("index"), index},
            {QStringLiteral("address"), address},
            {QStringLiteral("title"), title}};
}

// Single quotes: double ones in raw string throw moc off. Frame script globals: docShell over
// five-entry session history at index 3, messages kept.
const char *const FakeFrame = R"(
var listeners = {}, sent = [], went = [], broken = false;
var Components = { interfaces: { nsIWebNavigation: 'nsIWebNavigation' } };
var pages = ['https://a.example/', 'https://b.example/', 'https://c.example/',
             'https://d.example/', 'https://e.example/'];
var list = {
    index: 3,
    getEntryAtIndex: function (i) {
        return { URI: { spec: pages[i] }, title: 'Page ' + i };
    }
};
var docShell = {
    QueryInterface: function (name) {
        if (name !== 'nsIWebNavigation') { throw new Error('no ' + name); }
        return {
            get sessionHistory() {
                if (broken) { return { get legacySHistory() { throw new Error('SHIP'); } }; }
                return { legacySHistory: list };
            },
            gotoIndex: function (i) { went.push(i); }
        };
    }
};
function addMessageListener(name, listener) { listeners[name] = listener; }
function sendAsyncMessage(name, data) { sent.push({ name: name, data: data }); }
)";

} // namespace

class tst_tabhistory : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void entries();
    void rejects();
    void script();
    void favicons();
    void siteFavicons();
    void faviconsWithoutDatabase();

private:
    QVariantList entriesOf(const QVariantList &entries) const
    {
        return m_history->entries(QVariantMap{{QStringLiteral("entries"), entries}});
    }

    QScopedPointer<QTemporaryDir> m_dir;
    QScopedPointer<Storage> m_storage;
    QScopedPointer<TabHistory> m_history;
};

void tst_tabhistory::init()
{
    m_dir.reset(new QTemporaryDir);
    m_storage.reset(new Storage(m_dir->path()));
    m_history.reset(new TabHistory(*m_storage));
}

void tst_tabhistory::cleanup()
{
    m_history.reset();
    m_storage.reset();
    m_dir.reset();
}

void tst_tabhistory::entries()
{
    QCOMPARE(m_history->askMessage(), QStringLiteral("salama:tabhistory"));
    QCOMPARE(m_history->resultMessage(), QStringLiteral("salama:tabhistory-result"));
    QCOMPARE(m_history->goMessage(), QStringLiteral("salama:tabhistory-go"));

    // Reader view's own entry shows article address.
    const QString reader =
        QStringLiteral("data:text/html;charset=utf-8,") +
        QString::fromLatin1(QUrl::toPercentEncoding(QStringLiteral(
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<meta name=\"salama-reader\" content=\"https://news.example/story\">")));
    const QVariantList entries = entriesOf(
        {backEntry(2.0, QStringLiteral("https://c.example/"), QStringLiteral("  Page\n C ")),
         backEntry(qlonglong(1), reader, QStringLiteral("Story")),
         backEntry(0, QStringLiteral("https://a.example/"))});
    QCOMPARE(entries.count(), 3);
    const QVariantMap first = entries.at(0).toMap();
    QCOMPARE(first.value(QStringLiteral("index")), QVariant(2));
    QCOMPARE(first.value(QStringLiteral("address")).toString(),
             QStringLiteral("https://c.example/"));
    QCOMPARE(first.value(QStringLiteral("url")).toString(), QStringLiteral("https://c.example/"));
    QCOMPARE(first.value(QStringLiteral("title")).toString(), QStringLiteral("Page C"));
    QCOMPARE(first.count(), 5);
    const QVariantMap second = entries.at(1).toMap();
    QCOMPARE(second.value(QStringLiteral("index")), QVariant(1));
    QCOMPARE(second.value(QStringLiteral("address")).toString(), reader);
    QCOMPARE(second.value(QStringLiteral("url")).toString(),
             QStringLiteral("https://news.example/story"));
    QCOMPARE(entries.at(2).toMap().value(QStringLiteral("index")), QVariant(0));
    QCOMPARE(entries.at(2).toMap().value(QStringLiteral("title")).toString(), QString());

    QVariantList many;
    for (int i = 30; i >= 0; --i) {
        many.append(backEntry(i, QStringLiteral("https://example.com/%1").arg(i)));
    }
    const QVariantList capped = entriesOf(many);
    QCOMPARE(capped.count(), TabHistory::Limit);
    QCOMPARE(capped.first().toMap().value(QStringLiteral("index")), QVariant(30));
    QCOMPARE(capped.last().toMap().value(QStringLiteral("index")), QVariant(21));
}

void tst_tabhistory::rejects()
{
    const QString page = QStringLiteral("https://example.com/");
    QVERIFY(m_history->entries(QVariant()).isEmpty());
    QVERIFY(m_history->entries(QVariantMap()).isEmpty());
    QVERIFY(entriesOf({backEntry(QStringLiteral("1"), page)}).isEmpty());
    QVERIFY(entriesOf({backEntry(true, page)}).isEmpty());
    QVERIFY(entriesOf({backEntry(-1, page)}).isEmpty());
    QVERIFY(entriesOf({backEntry(1.5, page)}).isEmpty());
    QVERIFY(entriesOf({backEntry(qQNaN(), page)}).isEmpty());
    QVERIFY(entriesOf({backEntry(qInf(), page)}).isEmpty());
    QVERIFY(entriesOf({backEntry(1e12, page)}).isEmpty());
    QVERIFY(entriesOf({backEntry(1, QString())}).isEmpty());
    QVERIFY(entriesOf({backEntry(1, QStringLiteral("no scheme"))}).isEmpty());
    // Out of order: later entry not older than one before, dropped.
    const QVariantList ordered =
        entriesOf({backEntry(3, page), backEntry(3, page), backEntry(5, page), backEntry(1, page)});
    QCOMPARE(ordered.count(), 2);
    QCOMPARE(ordered.at(0).toMap().value(QStringLiteral("index")), QVariant(3));
    QCOMPARE(ordered.at(1).toMap().value(QStringLiteral("index")), QVariant(1));
    QCOMPARE(entriesOf({QVariant(), backEntry(0, page)}).count(), 1);
}

void tst_tabhistory::script()
{
    const QString url = m_history->scriptUrl();
    QVERIFY(url.startsWith(QLatin1String("data:application/javascript;charset=utf-8,")));
    QCOMPARE(QUrl::fromPercentEncoding(url.mid(url.indexOf(QLatin1Char(',')) + 1).toLatin1()),
             m_history->script());

    QJSEngine engine;
    QVERIFY(!engine.evaluate(QString::fromUtf8(FakeFrame)).isError());
    const QJSValue loaded = engine.evaluate(m_history->script());
    QVERIFY2(!loaded.isError(), qPrintable(loaded.toString()));
    const auto js = [&engine](const QString &code) { return engine.evaluate(code); };
    const auto answer = [&js]() {
        const QVariantMap sent = js(QStringLiteral("sent[sent.length - 1]")).toVariant().toMap();
        return sent;
    };

    // Entries before current, newest first; current and forward left out.
    js(QStringLiteral("listeners['salama:tabhistory']({})"));
    QVariantMap sent = answer();
    QCOMPARE(sent.value(QStringLiteral("name")).toString(), m_history->resultMessage());
    QVariantList entries =
        sent.value(QStringLiteral("data")).toMap().value(QStringLiteral("entries")).toList();
    QCOMPARE(entries.count(), 3);
    QCOMPARE(entries.at(0).toMap().value(QStringLiteral("index")).toInt(), 2);
    QCOMPARE(entries.at(0).toMap().value(QStringLiteral("address")).toString(),
             QStringLiteral("https://c.example/"));
    QCOMPARE(entries.at(0).toMap().value(QStringLiteral("title")).toString(),
             QStringLiteral("Page 2"));
    QCOMPARE(entries.at(2).toMap().value(QStringLiteral("index")).toInt(), 0);
    QCOMPARE(m_history->entries(sent.value(QStringLiteral("data"))).count(), 3);

    js(QStringLiteral(
        "list.index = 40; for (var i = 5; i <= 40; ++i) pages.push('https://x.example/' + i)"));
    js(QStringLiteral("listeners['salama:tabhistory']({})"));
    entries =
        answer().value(QStringLiteral("data")).toMap().value(QStringLiteral("entries")).toList();
    QCOMPARE(entries.count(), TabHistory::Limit);
    QCOMPARE(entries.first().toMap().value(QStringLiteral("index")).toInt(), 39);
    js(QStringLiteral("list.index = 3"));

    // Go only to entry still listed there, only backwards.
    const QString go =
        QStringLiteral("listeners['salama:tabhistory-go']({ data: { index: %1, address: '%2' } })");
    js(go.arg(1).arg(QStringLiteral("https://b.example/")));
    QCOMPARE(js(QStringLiteral("went.join()")).toString(), QStringLiteral("1"));
    js(go.arg(1).arg(QStringLiteral("https://other.example/")));
    js(go.arg(3).arg(QStringLiteral("https://d.example/")));
    js(go.arg(-1).arg(QStringLiteral("https://a.example/")));
    QCOMPARE(js(QStringLiteral("went.join()")).toString(), QStringLiteral("1"));

    // History held in parent process: empty answer, go ignored, nothing thrown at engine.
    js(QStringLiteral("broken = true"));
    QVERIFY(!js(QStringLiteral("listeners['salama:tabhistory']({})")).isError());
    QVERIFY(answer()
                .value(QStringLiteral("data"))
                .toMap()
                .value(QStringLiteral("entries"))
                .toList()
                .isEmpty());
    QVERIFY(!js(go.arg(1).arg(QStringLiteral("https://b.example/"))).isError());
    QCOMPARE(js(QStringLiteral("went.join()")).toString(), QStringLiteral("1"));
}

// Row icon from History; none for page not there or without one.
void tst_tabhistory::favicons()
{
    HistoryModel history(*m_storage);
    history.visit(QStringLiteral("https://a.example/"));
    history.updateFavicon(QStringLiteral("https://a.example/"),
                          QStringLiteral("https://a.example/icon.png"));
    history.visit(QStringLiteral("https://b.example/"));
    // Reader view entry: article's icon.
    const QString reader = QStringLiteral("data:text/html;charset=utf-8,") +
                           QString::fromLatin1(QUrl::toPercentEncoding(QStringLiteral(
                               "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                               "<meta name=\"salama-reader\" content=\"https://a.example/\">")));
    QCOMPARE(entriesOf({backEntry(0, reader)}).first().toMap().value(QStringLiteral("favicon")),
             QVariant(QStringLiteral("https://a.example/icon.png")));
    const QVariantList entries = entriesOf({backEntry(2, QStringLiteral("https://a.example/")),
                                            backEntry(1, QStringLiteral("https://b.example/")),
                                            backEntry(0, QStringLiteral("https://c.example/"))});
    QCOMPARE(entries.count(), 3);
    QCOMPARE(entries.at(0).toMap().value(QStringLiteral("favicon")).toString(),
             QStringLiteral("https://a.example/icon.png"));
    QVERIFY(entries.at(1).toMap().value(QStringLiteral("favicon")).toString().isEmpty());
    QVERIFY(entries.at(2).toMap().value(QStringLiteral("favicon")).toString().isEmpty());
}

// Page without own icon (script-made address, never loaded): site's latest. Other sites,
// look-alike hosts, no-host addresses get none.
void tst_tabhistory::siteFavicons()
{
    HistoryModel history(*m_storage);
    history.visit(QStringLiteral("https://a.example/old"));
    history.updateFavicon(QStringLiteral("https://a.example/old"), QStringLiteral("old.png"));
    history.visit(QStringLiteral("https://a.example/new"));
    history.updateFavicon(QStringLiteral("https://a.example/new"), QStringLiteral("new.png"));
    history.visit(QStringLiteral("https://a.example/pushed"));
    history.visit(QStringLiteral("https://aXexample.org/x"));
    history.updateFavicon(QStringLiteral("https://aXexample.org/x"), QStringLiteral("o.png"));
    history.visit(QStringLiteral("https://a.example.evil/x"));
    history.updateFavicon(QStringLiteral("https://a.example.evil/x"), QStringLiteral("e.png"));
    const auto icon = [this](const QString &url) {
        return entriesOf({backEntry(0, url)}).first().toMap().value(QStringLiteral("favicon"));
    };
    QCOMPARE(icon(QStringLiteral("https://a.example/pushed")), QVariant(QStringLiteral("new.png")));
    QCOMPARE(icon(QStringLiteral("https://a.example/old")), QVariant(QStringLiteral("old.png")));
    QCOMPARE(icon(QStringLiteral("https://a.example/never")), QVariant(QStringLiteral("new.png")));
    QCOMPARE(icon(QStringLiteral("http://a.example/pushed")), QVariant(QString()));
    QCOMPARE(icon(QStringLiteral("https://aXexample.org/y")), QVariant(QStringLiteral("o.png")));
    QCOMPARE(icon(QStringLiteral("https://a_example.org/y")), QVariant(QString()));
    QCOMPARE(icon(QStringLiteral("https://b.example/")), QVariant(QString()));
    QCOMPARE(icon(QStringLiteral("about:blank")), QVariant(QString()));
}

// Broken database: rows still listed, no icon.
void tst_tabhistory::faviconsWithoutDatabase()
{
    QSqlQuery(m_storage->database()).exec(QStringLiteral("DROP TABLE browser_history"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("^TabHistory:")));
    const QVariantList entries = entriesOf({backEntry(0, QStringLiteral("https://a.example/"))});
    QCOMPARE(entries.count(), 1);
    QVERIFY(entries.first().toMap().value(QStringLiteral("favicon")).toString().isEmpty());
}

QTEST_GUILESS_MAIN(tst_tabhistory)
#include "tst_tabhistory.moc"
