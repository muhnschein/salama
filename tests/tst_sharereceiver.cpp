// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "share/ShareReceiver.h"

#include <QSignalSpy>
#include <QtTest>

using Salama::ShareReceiver;

// Link shared from another app's share sheet. Tests D-Bus payload parsing; call itself
// needs phone session bus.
class tst_sharereceiver : public QObject
{
    Q_OBJECT

private slots:
    void sharedUrl_data();
    void sharedUrl();
    void heldUntilReady();
    void notALinkIsDropped();
    void unwrapLeavesPlainValues();
    void noBusNoService();
};

namespace {

// Call as sailfish-browser and own ShareAction make it (captured on device by
// harbour-nextmarks): one resource, url in status.
QVariantMap browserShare(const QString &url)
{
    return {{QStringLiteral("mimeType"), QStringLiteral("text/x-url")},
            {QStringLiteral("resources"),
             QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("text/x-url")},
                                      {QStringLiteral("status"), url},
                                      {QStringLiteral("linkTitle"), QStringLiteral("A page")}}}},
            {QStringLiteral("title"), QStringLiteral("Share link")}};
}

QVariantMap dataShare(const QVariant &data)
{
    return {{QStringLiteral("resources"),
             QVariantList{QVariantMap{{QStringLiteral("name"), QStringLiteral("link")},
                                      {QStringLiteral("data"), data}}}}};
}

} // namespace

void tst_sharereceiver::sharedUrl_data()
{
    QTest::addColumn<QVariantMap>("arguments");
    QTest::addColumn<QString>("expected");

    QTest::newRow("browser's link") << browserShare(QStringLiteral("https://yle.fi/uutiset"))
                                    << QStringLiteral("https://yle.fi/uutiset");
    QTest::newRow("plain http") << browserShare(QStringLiteral("http://example.org/"))
                                << QStringLiteral("http://example.org/");
    QTest::newRow("spaces round it") << browserShare(QStringLiteral("  https://example.org/a \n"))
                                     << QStringLiteral("https://example.org/a");
    QTest::newRow("as data") << dataShare(QByteArray("https://example.org/b"))
                             << QStringLiteral("https://example.org/b");
    QTest::newRow("as text data") << dataShare(QStringLiteral("https://example.org/c"))
                                  << QStringLiteral("https://example.org/c");
    QTest::newRow("wrapped once more")
        << QVariantMap{{QStringLiteral("resources"),
                        QVariantList{
                            QVariantList{browserShare(QStringLiteral("https://example.org/d"))
                                             .value(QStringLiteral("resources"))
                                             .toList()
                                             .first()}}}}
        << QStringLiteral("https://example.org/d");
    QTest::newRow("words") << dataShare(QStringLiteral("Meet at six by the station")) << QString();
    QTest::newRow("words and a link")
        << dataShare(QStringLiteral("Look https://example.org/")) << QString();
    QTest::newRow("file") << browserShare(QStringLiteral("file:///etc/passwd")) << QString();
    QTest::newRow("javascript") << browserShare(QStringLiteral("javascript:alert(1)")) << QString();
    QTest::newRow("ftp, with a host")
        << browserShare(QStringLiteral("ftp://example.org/file")) << QString();
    QTest::newRow("tel") << browserShare(QStringLiteral("tel:+358401234567")) << QString();
    QTest::newRow("no host") << browserShare(QStringLiteral("https://")) << QString();
    QTest::newRow("no resources") << QVariantMap{} << QString();
    QTest::newRow("empty resources")
        << QVariantMap{{QStringLiteral("resources"), QVariantList{}}} << QString();
    QTest::newRow("empty wrapping")
        << QVariantMap{{QStringLiteral("resources"), QVariantList{QVariantList{}}}} << QString();
}

void tst_sharereceiver::sharedUrl()
{
    QFETCH(QVariantMap, arguments);
    QFETCH(QString, expected);
    QCOMPARE(ShareReceiver::sharedUrl(arguments), expected);
}

// Call answered at once; launch link waits until window listens; later ones passed straight on.
void tst_sharereceiver::heldUntilReady()
{
    ShareReceiver receiver;
    QSignalSpy spy(&receiver, &ShareReceiver::linkShared);
    receiver.receive(browserShare(QStringLiteral("https://first.example/")));
    QCOMPARE(spy.count(), 0);
    receiver.setReady();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().first().toString(), QStringLiteral("https://first.example/"));
    receiver.setReady();
    QCOMPARE(spy.count(), 0);
    receiver.receive(browserShare(QStringLiteral("https://second.example/")));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().first().toString(), QStringLiteral("https://second.example/"));
}

void tst_sharereceiver::notALinkIsDropped()
{
    ShareReceiver receiver;
    QSignalSpy spy(&receiver, &ShareReceiver::linkShared);
    receiver.receive(dataShare(QStringLiteral("not a link")));
    receiver.setReady();
    receiver.receive(dataShare(QStringLiteral("still not")));
    QCOMPARE(spy.count(), 0);
}

void tst_sharereceiver::unwrapLeavesPlainValues()
{
    const QVariantMap map{{QStringLiteral("a"), 1}};
    QCOMPARE(ShareReceiver::unwrap(map), QVariant(map));
    QCOMPARE(ShareReceiver::unwrap(QStringLiteral("x")), QVariant(QStringLiteral("x")));
}

// No session bus on host: refusal, not failure.
void tst_sharereceiver::noBusNoService()
{
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/salama-test-bus");
    ShareReceiver receiver;
    QVERIFY(!receiver.registerService());
}

QTEST_GUILESS_MAIN(tst_sharereceiver)
#include "tst_sharereceiver.moc"
