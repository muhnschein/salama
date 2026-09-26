// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "notifications/NotificationPermissions.h"
#include "notifications/WebNotifications.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJSEngine>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::NotificationPermissions;
using Salama::WebNotifications;

namespace {

const QString Chat = QStringLiteral("https://chat.example");
const QString Page = QStringLiteral("p1");

// What embedlite-components' ContentPermissionManager.js sends on "embed:perms:all", as
// qtmozembed hands it over: a list of maps, every number a double.
QVariantMap permission(const QString &type, const QString &uri, int capability, int expireType = 0)
{
    return {
        {QStringLiteral("type"), type},
        {QStringLiteral("uri"), uri},
        {QStringLiteral("capability"), static_cast<double>(capability)},
        {QStringLiteral("expireType"), static_cast<double>(expireType)},
    };
}

QVariantMap notificationPermission(const QString &uri, int capability, int expireType = 0)
{
    return permission(QStringLiteral("desktop-notification"), uri, capability, expireType);
}

// What the frame script sends from a page: the page's message, as JSON, with the origin
// and the permission Gecko holds.
QVariantMap relayed(const QVariantMap &message, const QString &origin = Chat,
                    const QString &permission = QStringLiteral("default"))
{
    QVariantMap addressed = message;
    if (!addressed.contains(QStringLiteral("page"))) {
        addressed.insert(QStringLiteral("page"), Page);
    }
    return {
        {QStringLiteral("origin"), origin},
        {QStringLiteral("permission"), permission},
        {QStringLiteral("detail"),
         QString::fromUtf8(QJsonDocument::fromVariant(addressed).toJson(QJsonDocument::Compact))},
    };
}

QVariantMap requestMessage(int id)
{
    return {{QStringLiteral("type"), QStringLiteral("request")}, {QStringLiteral("id"), id}};
}

QVariantMap showMessage(int id, const QString &title, const QString &tag = QString(),
                        const QString &icon = QString())
{
    QVariantMap message{
        {QStringLiteral("type"), QStringLiteral("show")},
        {QStringLiteral("id"), static_cast<double>(id)},
        {QStringLiteral("title"), title},
        {QStringLiteral("body"), QStringLiteral("body of ") + title},
        {QStringLiteral("tag"), tag},
    };
    if (!icon.isEmpty()) {
        message.insert(QStringLiteral("icon"), icon);
    }
    return message;
}

// The message a reply script hands the page, read back out of the script.
QVariantMap replyOf(const QString &script)
{
    QJSEngine engine;
    engine.evaluate(QStringLiteral(
        "var window = { sent: null, dispatchEvent: function (e) { this.sent = e.detail; } };"
        "function CustomEvent(type, options) { this.type = type; this.detail = options.detail; }"));
    const QJSValue result = engine.evaluate(QStringLiteral("(function () { %1 })()").arg(script));
    if (result.isError() || !result.toBool()) {
        return {{QStringLiteral("error"), result.toString()}};
    }
    return QJsonDocument::fromJson(
               engine.evaluate(QStringLiteral("window.sent")).toString().toUtf8())
        .toVariant()
        .toMap();
}

// Every reply sent to a tab, in order, as "type:id:detail".
QStringList replies(const QSignalSpy &spy, int tabId)
{
    QStringList list;
    for (const QList<QVariant> &arguments : spy) {
        if (arguments.at(0).toInt() != tabId) {
            continue;
        }
        const QVariantMap reply = replyOf(arguments.at(1).toString());
        QString line = reply.value(QStringLiteral("type")).toString() + QLatin1Char(':') +
                       QString::number(reply.value(QStringLiteral("id")).toInt());
        if (reply.contains(QStringLiteral("permission"))) {
            line += QLatin1Char(':') + reply.value(QStringLiteral("permission")).toString();
        }
        if (reply.value(QStringLiteral("page")).toString() != Page) {
            line += QStringLiteral(":page=") + reply.value(QStringLiteral("page")).toString();
        }
        list.append(line);
    }
    return list;
}

QString pngDataUrl(int width, int height)
{
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(bytes.toBase64());
}

// A page for the page script, in a JavaScript engine rather than a browser's: the parts
// of the DOM it touches, the timers run by hand. `permission` is what Gecko says; a
// trusted event is what a touch is; `images` are what it loads, loaded or failed by the
// test; `posted` is what it says to the frame script.
const char *const FakePage = R"(
var permission = 'default', posted = [], timers = [], images = [], drawn = [], errors = [];
var tainted = false;
function EventTarget() { this.listeners = {}; }
EventTarget.prototype.addEventListener = function (type, listener) {
  (this.listeners[type] = this.listeners[type] || []).push(listener);
};
EventTarget.prototype.dispatchEvent = function (event) {
  event.target = this;
  var list = (this.listeners[event.type] || []).slice();
  for (var i = 0; i < list.length; ++i) {
    try { list[i].call(this, event); } catch (e) { errors.push(String(e)); }
  }
  return !event.defaultPrevented;
};
function Event(type, options) {
  this.type = type;
  this.cancelable = !!(options && options.cancelable);
  this.isTrusted = !!(options && options.trusted);
  this.defaultPrevented = false;
}
Event.prototype.preventDefault = function () { if (this.cancelable) { this.defaultPrevented = true; } };
function CustomEvent(type, options) { Event.call(this, type, options); this.detail = options.detail; }
CustomEvent.prototype = Object.create(Event.prototype);
function URL(url, base) {
  if (url === 'http://[bad') { throw new TypeError('bad url'); }
  if (/^[a-z][a-z0-9+.-]*:/i.test(url)) { this.href = url; }
  else if (url.charAt(0) === '/') { this.href = base.replace(/^([a-z]+:\/\/[^\/]+).*$/i, '$1') + url; }
  else { this.href = base.replace(/[^\/]*$/, '') + url; }
}
function structuredClone(value) { return JSON.parse(JSON.stringify(value)); }
function setTimeout(callback, delay) { timers.push({ callback: callback, delay: delay }); }
function runTimers() {
  var list = timers;
  timers = [];
  list.forEach(function (timer) { timer.callback(); });
}
function Image() { images.push(this); }
var canvas = {
  getContext: function () {
    return { drawImage: function (image, x, y, width, height) { drawn.push(width + 'x' + height); } };
  },
  toDataURL: function (type) {
    if (tainted) { throw new Error('SecurityError'); }
    return 'data:' + type + ';base64,' + canvas.width + 'x' + canvas.height;
  }
};
var document = { baseURI: 'https://chat.example/room/', createElement: function () { return canvas; } };
function NativeNotification() { throw new TypeError('native'); }
Object.defineProperty(NativeNotification, 'permission', { get: function () { return permission; } });
function ServiceWorkerRegistration(scope) { this.scope = scope; }
ServiceWorkerRegistration.prototype.showNotification = function () { return 'native'; };
ServiceWorkerRegistration.prototype.getNotifications = function () { return 'native'; };
var window = new EventTarget();
window.Notification = NativeNotification;
window.ServiceWorkerRegistration = ServiceWorkerRegistration;
window.addEventListener('salama-notification', function (event) { posted.push(JSON.parse(event.detail)); });
function touch() { window.dispatchEvent(new Event('touchend', { trusted: true })); }
function install(script) { return new Function(script)(); }
function reply(script) { return new Function(script)(); }
function last() { return posted[posted.length - 1]; }
)";

// The frame script's world: the message manager's own addEventListener and
// sendAsyncMessage, and the content window. `fire` hands a listener an event.
const char *const FakeFrame = R"(
var listeners = {}, sent = [];
function addEventListener(type, listener, capture, untrusted) {
  listeners[type] = { listener: listener, capture: capture, untrusted: untrusted };
}
function sendAsyncMessage(name, data) { sent.push({ name: name, data: data }); }
var nativePermission = 'granted';
var content = {
  location: { origin: 'https://chat.example' },
  document: { title: 'top' },
  Notification: {}
};
Object.defineProperty(content.Notification, 'permission', {
  get: function () { if (nativePermission === null) { throw new Error('gone'); } return nativePermission; }
});
function fire(type, target, detail) { listeners[type].listener({ target: target, detail: detail }); }
)";

} // namespace

class tst_webnotifications : public QObject
{
    Q_OBJECT

private slots:
    void origins();
    void permissionList();
    void permissionListIgnoresTheRest();
    void permissionChanges();
    void defaultPreference();
    void automaticDenialTakenBack();

    void strings();
    void requestAsksOncePerTab();
    void requestAnswers_data();
    void requestAnswers();
    void decidedSitesAnswerAtOnce();
    void showIsGatedByThePermission();
    void showPublishes();
    void aTagReplaces();
    void closeFromThePage();
    void unloadTakesThePageWithIt();
    void tapAndDismiss();
    void closeAll();
    void ignoresWhatItDoesNotKnow();
    void icons();
    void popupOpening();

    void relayScript();
    void pageScriptInstallsOnce();
    void pageScriptPermission();
    void pageScriptRefusal();
    void pageScriptNotifications();
    void pageScriptIcons();
    void pageScriptServiceWorker();
};

void tst_webnotifications::origins()
{
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("https://Chat.Example/room?x=1")),
             Chat);
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("http://chat.example:80/")),
             QStringLiteral("http://chat.example"));
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("https://chat.example:443")), Chat);
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("https://chat.example:8443/a")),
             QStringLiteral("https://chat.example:8443"));
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("https://[::1]:8080/")),
             QStringLiteral("https://[::1]:8080"));
    // The host in ASCII, as Gecko's principals have it; shown to the reader as it reads.
    const QString ace = NotificationPermissions::originOf(QStringLiteral("https://ääkkönen.fi/"));
    QCOMPARE(ace, QStringLiteral("https://xn--kknen-fraa0m.fi"));
    QCOMPARE(NotificationPermissions::hostOf(ace), QStringLiteral("ääkkönen.fi"));
    QCOMPARE(NotificationPermissions::hostOf(Chat), QStringLiteral("chat.example"));
    // Nothing but a site on the web has an origin to allow.
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("data:text/html,x")), QString());
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("file:///tmp/a.html")), QString());
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("about:blank")), QString());
    QCOMPARE(NotificationPermissions::originOf(QString()), QString());
    QCOMPARE(NotificationPermissions::originOf(QStringLiteral("https:///path")), QString());
}

void tst_webnotifications::permissionList()
{
    NotificationPermissions permissions;
    QCOMPARE(permissions.topic(), QStringLiteral("embed:perms:all"));
    QSignalSpy count(&permissions, &NotificationPermissions::countChanged);
    QSignalSpy requests(&permissions, &NotificationPermissions::engineRequest);

    // Asked for as the model is refreshed.
    permissions.refresh();
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.at(0).at(0).toString(), QStringLiteral("embedui:perms"));
    QCOMPARE(requests.at(0).at(1).toMap().value(QStringLiteral("msg")).toString(),
             QStringLiteral("get-all"));

    // Sorted by host; the origin's attributes after a caret are not the site's.
    permissions.observe(QStringLiteral("embed:perms:all"),
                        QVariantList{
                            notificationPermission(QStringLiteral("https://news.example"), 2),
                            notificationPermission(QStringLiteral("https://chat.example^u=1"), 1),
                            notificationPermission(QStringLiteral("https://b.example:8443"), 1),
                        });
    QCOMPARE(permissions.rowCount(), 3);
    QCOMPARE(count.count(), 1);
    const auto at = [&permissions](int row, int role) {
        return permissions.data(permissions.index(row), role);
    };
    QCOMPARE(at(0, roleId(NotificationPermissions::Role::Origin)).toString(),
             QStringLiteral("https://b.example:8443"));
    QCOMPARE(at(0, roleId(NotificationPermissions::Role::Host)).toString(),
             QStringLiteral("b.example"));
    QCOMPARE(at(1, roleId(NotificationPermissions::Role::Origin)).toString(), Chat);
    QVERIFY(at(1, roleId(NotificationPermissions::Role::Allowed)).toBool());
    QCOMPARE(at(2, roleId(NotificationPermissions::Role::Host)).toString(),
             QStringLiteral("news.example"));
    QVERIFY(!at(2, roleId(NotificationPermissions::Role::Allowed)).toBool());
    QVERIFY(!at(3, roleId(NotificationPermissions::Role::Origin)).isValid());
    QVERIFY(!at(0, Qt::DisplayRole).isValid());
    QCOMPARE(permissions.roleNames().value(roleId(NotificationPermissions::Role::Allowed)),
             QByteArray("allowed"));
    QCOMPARE(permissions.rowCount(permissions.index(0)), 0);

    // Any address of a site is the site.
    QVERIFY(permissions.isAllowed(QStringLiteral("https://chat.example/room/42")));
    QVERIFY(!permissions.isAllowed(QStringLiteral("https://news.example/")));
    QVERIFY(permissions.isBlocked(QStringLiteral("https://news.example")));
    QVERIFY(!permissions.isBlocked(Chat));
    QVERIFY(!permissions.isAllowed(QStringLiteral("https://other.example/")));
    QVERIFY(!permissions.isAllowed(QStringLiteral("data:text/html,x")));

    // As a string too, which is how qtmozembed hands over what it could not read.
    permissions.observe(QStringLiteral("embed:perms:all"),
                        QStringLiteral("[{\"type\":\"desktop-notification\","
                                       "\"uri\":\"https://chat.example\",\"capability\":2,"
                                       "\"expireType\":0}]"));
    QCOMPARE(permissions.rowCount(), 1);
    QVERIFY(permissions.isBlocked(Chat));
    QCOMPARE(count.count(), 2);
}

void tst_webnotifications::permissionListIgnoresTheRest()
{
    NotificationPermissions permissions;
    permissions.observe(
        QStringLiteral("embed:perms:all"),
        QVariantList{
            // Another permission; one for the session, the platform's; one to prompt;
            // one of a page that is no site; a repeat.
            permission(QStringLiteral("geolocation"), QStringLiteral("https://maps.example"), 1),
            notificationPermission(QStringLiteral("https://session.example"), 2, 1),
            notificationPermission(QStringLiteral("https://prompt.example"), 3),
            notificationPermission(QStringLiteral("file:///tmp"), 1),
            notificationPermission(Chat, 1),
            notificationPermission(Chat, 2),
            QVariantMap{{QStringLiteral("type"), QStringLiteral("desktop-notification")},
                        {QStringLiteral("uri"), QStringLiteral("https://odd.example")},
                        {QStringLiteral("capability"), QStringLiteral("1")},
                        {QStringLiteral("expireType"), QStringLiteral("0")}},
        });
    QCOMPARE(permissions.rowCount(), 1);
    QVERIFY(permissions.isAllowed(Chat));

    // Another topic is not the list.
    permissions.observe(QStringLiteral("embed:download"), QVariantList{});
    QCOMPARE(permissions.rowCount(), 1);
}

void tst_webnotifications::permissionChanges()
{
    NotificationPermissions permissions;
    QSignalSpy requests(&permissions, &NotificationPermissions::engineRequest);
    QSignalSpy count(&permissions, &NotificationPermissions::countChanged);
    QSignalSpy changed(&permissions, &QAbstractItemModel::dataChanged);
    const auto request = [&requests](int index) { return requests.at(index).at(1).toMap(); };

    // Allowed for good, as the engine's permission manager keeps it.
    permissions.setAllowed(QStringLiteral("https://news.example/today"), true);
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.at(0).at(0).toString(), QStringLiteral("embedui:perms"));
    QCOMPARE(request(0).value(QStringLiteral("msg")).toString(), QStringLiteral("add"));
    QCOMPARE(request(0).value(QStringLiteral("uri")).toString(),
             QStringLiteral("https://news.example"));
    QCOMPARE(request(0).value(QStringLiteral("type")).toString(),
             QStringLiteral("desktop-notification"));
    QCOMPARE(request(0).value(QStringLiteral("permission")).toInt(), 1);
    QCOMPARE(request(0).value(QStringLiteral("expireType")).toInt(), 0);
    QCOMPARE(permissions.rowCount(), 1);
    QCOMPARE(count.count(), 1);

    // In its place by host.
    permissions.setAllowed(Chat, false);
    QCOMPARE(request(1).value(QStringLiteral("permission")).toInt(), 2);
    QCOMPARE(permissions.data(permissions.index(0), roleId(NotificationPermissions::Role::Origin))
                 .toString(),
             Chat);
    QVERIFY(permissions.isBlocked(Chat));

    // Changed in place; the same again changes nothing but tells the engine.
    permissions.setAllowed(Chat, true);
    QCOMPARE(changed.count(), 1);
    QVERIFY(permissions.isAllowed(Chat));
    permissions.setAllowed(Chat, true);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(requests.count(), 4);
    QCOMPARE(count.count(), 2);

    // Removed: asked about again.
    permissions.remove(QStringLiteral("https://chat.example/room"));
    QCOMPARE(request(4).value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(request(4).value(QStringLiteral("uri")).toString(), Chat);
    QCOMPARE(permissions.rowCount(), 1);
    QVERIFY(!permissions.isAllowed(Chat));
    QCOMPARE(count.count(), 3);

    // Nothing to remove, and nothing a site: nothing said.
    permissions.remove(Chat);
    permissions.setAllowed(QStringLiteral("about:blank"), true);
    QCOMPARE(requests.count(), 5);
}

void tst_webnotifications::defaultPreference()
{
    const QVariantMap asked = NotificationPermissions::defaultPreference(false);
    QCOMPARE(asked.value(QStringLiteral("name")).toString(),
             QStringLiteral("permissions.default.desktop-notification"));
    QCOMPARE(asked.value(QStringLiteral("value")).toInt(), 0);
    QCOMPARE(
        NotificationPermissions::defaultPreference(true).value(QStringLiteral("value")).toInt(), 2);
}

void tst_webnotifications::automaticDenialTakenBack()
{
    NotificationPermissions permissions;
    QSignalSpy requests(&permissions, &NotificationPermissions::engineRequest);
    permissions.undoAutomaticDenial(QStringLiteral("https://chat.example/room"));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.at(0).at(1).toMap().value(QStringLiteral("msg")).toString(),
             QStringLiteral("remove"));
    QCOMPARE(requests.at(0).at(1).toMap().value(QStringLiteral("uri")).toString(), Chat);
    // A site decided on is the reader's, and nothing is a site's but a site's.
    permissions.setAllowed(Chat, false);
    permissions.undoAutomaticDenial(Chat);
    permissions.undoAutomaticDenial(QStringLiteral("data:text/html,x"));
    QCOMPARE(requests.count(), 2);
    QVERIFY(permissions.isBlocked(Chat));
}

void tst_webnotifications::strings()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QCOMPARE(notifications.messageName(), QStringLiteral("salama:notification"));
    // The frame script, as a data: url the engine loads, which is the script itself.
    const QString url = notifications.relayScriptUrl();
    QVERIFY(url.startsWith(QLatin1String("data:application/javascript;charset=utf-8,")));
    QCOMPARE(QUrl::fromPercentEncoding(url.mid(url.indexOf(QLatin1Char(',')) + 1).toLatin1()),
             notifications.relayScript());
    QVERIFY(notifications.relayScript().contains(QLatin1String("\"salama:notification\"")));
    // The page script is the body of a function, as every script the engine runs.
    const QString page = notifications.pageScript();
    QVERIFY(page.contains(QLatin1String("return true;")));
    QVERIFY(page.contains(QLatin1String("var ICON = 256, WAIT = 3000, ACTIVATION = 5000;")));
    QVERIFY(!page.contains(QLatin1String("%")));

    // A reply is the message as a string, whatever it holds.
    const QVariantMap message{{QStringLiteral("type"), QStringLiteral("show")},
                              {QStringLiteral("text"), QStringLiteral("'\"\\\n </script>")}};
    QCOMPARE(replyOf(WebNotifications::replyScript(message)), message);
}

void tst_webnotifications::requestAsksOncePerTab()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QSignalSpy asked(&notifications, &WebNotifications::permissionRequested);
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);

    notifications.receive(1, relayed(requestMessage(1)));
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.at(0).at(0).toInt(), 1);
    QCOMPARE(asked.at(0).at(1).toString(), QStringLiteral("chat.example"));
    // The same page asking again waits for the same answer; another tab asks for itself.
    notifications.receive(1, relayed(requestMessage(2)));
    notifications.receive(2, relayed(requestMessage(1)));
    QCOMPARE(asked.count(), 2);
    QCOMPARE(asked.at(1).at(0).toInt(), 2);
    QCOMPARE(pages.count(), 0);

    notifications.answer(1, WebNotifications::NotNow);
    QCOMPARE(replies(pages, 1), (QStringList{QStringLiteral("permission:1:denied"),
                                             QStringLiteral("permission:2:denied")}));
    QCOMPARE(replies(pages, 2), QStringList());
    // Answered once.
    notifications.answer(1, WebNotifications::Allow);
    QCOMPARE(pages.count(), 2);
    QCOMPARE(permissions.rowCount(), 0);
}

void tst_webnotifications::requestAnswers_data()
{
    QTest::addColumn<int>("decision");
    QTest::addColumn<QString>("answer");
    QTest::addColumn<int>("sites");
    QTest::addColumn<bool>("allowed");
    QTest::newRow("allow") << int(WebNotifications::Allow) << QStringLiteral("granted") << 1
                           << true;
    QTest::newRow("block") << int(WebNotifications::Block) << QStringLiteral("denied") << 1
                           << false;
    QTest::newRow("not now") << int(WebNotifications::NotNow) << QStringLiteral("denied") << 0
                             << false;
    // Nothing else is an answer but not now.
    QTest::newRow("out of range") << 7 << QStringLiteral("denied") << 0 << false;
}

void tst_webnotifications::requestAnswers()
{
    QFETCH(int, decision);
    QFETCH(QString, answer);
    QFETCH(int, sites);
    QFETCH(bool, allowed);
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);
    QSignalSpy engine(&permissions, &NotificationPermissions::engineRequest);

    notifications.receive(3, relayed(requestMessage(4)));
    notifications.answer(3, decision);
    QCOMPARE(replies(pages, 3), QStringList{QStringLiteral("permission:4:") + answer});
    QCOMPARE(permissions.rowCount(), sites);
    QCOMPARE(engine.count(), sites);
    QCOMPARE(permissions.isAllowed(Chat), allowed);
}

void tst_webnotifications::decidedSitesAnswerAtOnce()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QSignalSpy asked(&notifications, &WebNotifications::permissionRequested);
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);
    permissions.setAllowed(Chat, true);
    permissions.setAllowed(QStringLiteral("https://news.example"), false);

    notifications.receive(1, relayed(requestMessage(1)));
    notifications.receive(1, relayed(requestMessage(2), QStringLiteral("https://news.example")));
    QCOMPARE(asked.count(), 0);
    QCOMPARE(replies(pages, 1), (QStringList{QStringLiteral("permission:1:granted"),
                                             QStringLiteral("permission:2:denied")}));
}

void tst_webnotifications::showIsGatedByThePermission()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QSignalSpy published(&notifications, &WebNotifications::publishRequested);
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);

    // Gecko's word is the page's permission.
    notifications.receive(
        1, relayed(showMessage(1, QStringLiteral("a")), Chat, QStringLiteral("granted")));
    // Not asked, or refused: an error for the page, nothing shown.
    notifications.receive(1, relayed(showMessage(2, QStringLiteral("b"))));
    notifications.receive(
        1, relayed(showMessage(3, QStringLiteral("c")), Chat, QStringLiteral("denied")));
    // Allowed here before the engine has written it down.
    permissions.setAllowed(Chat, true);
    notifications.receive(1, relayed(showMessage(4, QStringLiteral("d"))));
    // Never over Gecko's refusal.
    notifications.receive(
        1, relayed(showMessage(5, QStringLiteral("e")), Chat, QStringLiteral("denied")));
    QCOMPARE(published.count(), 2);
    QCOMPARE(replies(pages, 1), (QStringList{QStringLiteral("show:1"), QStringLiteral("error:2"),
                                             QStringLiteral("error:3"), QStringLiteral("show:4"),
                                             QStringLiteral("error:5")}));
    QCOMPARE(notifications.keys().count(), 2);
}

void tst_webnotifications::showPublishes()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QSignalSpy published(&notifications, &WebNotifications::publishRequested);

    const QString longTitle = QString(WebNotifications::TextLimit + 5, QLatin1Char('x'));
    QVariantMap message = showMessage(7, longTitle);
    message.insert(QStringLiteral("body"), QStringLiteral("See you"));
    notifications.receive(2, relayed(message, QStringLiteral("https://chat.example:8443"),
                                     QStringLiteral("granted")));
    QCOMPARE(published.count(), 1);
    const int key = published.at(0).at(0).toInt();
    QVERIFY(key > 0);
    QCOMPARE(notifications.tabOf(key), 2);
    const QVariantMap fields = published.at(0).at(1).toMap();
    QCOMPARE(fields.value(QStringLiteral("summary")).toString(),
             longTitle.left(WebNotifications::TextLimit) + QStringLiteral(" …"));
    QCOMPARE(fields.value(QStringLiteral("body")).toString(), QStringLiteral("See you"));
    // The site says who it is from, where the page cannot say otherwise.
    QCOMPARE(fields.value(QStringLiteral("subText")).toString(), QStringLiteral("chat.example"));
    QCOMPARE(fields.value(QStringLiteral("icon")).toString(), QString());
}

void tst_webnotifications::aTagReplaces()
{
    NotificationPermissions permissions;
    permissions.setAllowed(Chat, true);
    WebNotifications notifications(&permissions, QString());
    QSignalSpy published(&notifications, &WebNotifications::publishRequested);
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);

    notifications.receive(1,
                          relayed(showMessage(1, QStringLiteral("one"), QStringLiteral("room"))));
    notifications.receive(1,
                          relayed(showMessage(2, QStringLiteral("two"), QStringLiteral("room"))));
    // Another tab of the same site, the same tag: in the same place.
    notifications.receive(4,
                          relayed(showMessage(1, QStringLiteral("three"), QStringLiteral("room"))));
    // Another tag, no tag, another site: each its own.
    notifications.receive(1, relayed(showMessage(3, QStringLiteral("four"), QStringLiteral("dm"))));
    notifications.receive(1, relayed(showMessage(4, QStringLiteral("five"))));
    notifications.receive(1, relayed(showMessage(5, QStringLiteral("six"))));
    notifications.receive(
        1, relayed(showMessage(6, QStringLiteral("seven"), QStringLiteral("room")),
                   QStringLiteral("https://other.example"), QStringLiteral("granted")));
    QCOMPARE(published.count(), 7);
    const auto keyOf = [&published](int index) { return published.at(index).at(0).toInt(); };
    QCOMPARE(keyOf(1), keyOf(0));
    QCOMPARE(keyOf(2), keyOf(0));
    QCOMPARE(notifications.tabOf(keyOf(0)), 4);
    QCOMPARE(QSet<int>({keyOf(0), keyOf(3), keyOf(4), keyOf(5), keyOf(6)}).count(), 5);
    QCOMPARE(notifications.keys().count(), 5);
    // The page whose notification was replaced hears nothing of it.
    QVERIFY(!replies(pages, 1).contains(QStringLiteral("close:1")));
}

void tst_webnotifications::closeFromThePage()
{
    NotificationPermissions permissions;
    permissions.setAllowed(Chat, true);
    WebNotifications notifications(&permissions, QString());
    QSignalSpy published(&notifications, &WebNotifications::publishRequested);
    QSignalSpy closed(&notifications, &WebNotifications::closeRequested);
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);

    notifications.receive(1, relayed(showMessage(1, QStringLiteral("a"))));
    const int key = published.at(0).at(0).toInt();
    // Another page's number, another tab's: not this one.
    notifications.receive(1, relayed({{QStringLiteral("type"), QStringLiteral("close")},
                                      {QStringLiteral("id"), 1},
                                      {QStringLiteral("page"), QStringLiteral("p2")}}));
    notifications.receive(
        2, relayed({{QStringLiteral("type"), QStringLiteral("close")}, {QStringLiteral("id"), 1}}));
    QCOMPARE(closed.count(), 0);
    notifications.receive(
        1, relayed({{QStringLiteral("type"), QStringLiteral("close")}, {QStringLiteral("id"), 1}}));
    QCOMPARE(closed.count(), 1);
    QCOMPARE(closed.at(0).at(0).toInt(), key);
    QVERIFY(notifications.keys().isEmpty());
    // The page closed it, and says so itself: no reply.
    QCOMPARE(replies(pages, 1), QStringList{QStringLiteral("show:1")});
    // The platform saying it went, after, is nothing new.
    notifications.closed(key);
    QCOMPARE(pages.count(), 1);
}

void tst_webnotifications::unloadTakesThePageWithIt()
{
    NotificationPermissions permissions;
    permissions.setAllowed(Chat, true);
    WebNotifications notifications(&permissions, QString());
    QSignalSpy published(&notifications, &WebNotifications::publishRequested);
    QSignalSpy closed(&notifications, &WebNotifications::closeRequested);
    QSignalSpy withdrawn(&notifications, &WebNotifications::permissionWithdrawn);

    notifications.receive(1, relayed(showMessage(1, QStringLiteral("a"))));
    notifications.receive(1, relayed(showMessage(2, QStringLiteral("b"))));
    notifications.receive(2, relayed(showMessage(1, QStringLiteral("c"))));
    permissions.remove(Chat);
    notifications.receive(1, relayed(requestMessage(3)));

    // The frame script's word as the document goes, whatever the origin.
    notifications.receive(
        1, relayed({{QStringLiteral("type"), QStringLiteral("unload")}}, QStringLiteral("null")));
    QCOMPARE(closed.count(), 2);
    QCOMPARE(closed.at(0).at(0).toInt(), published.at(0).at(0).toInt());
    QCOMPARE(closed.at(1).at(0).toInt(), published.at(1).at(0).toInt());
    QCOMPARE(withdrawn.count(), 1);
    QCOMPARE(withdrawn.at(0).at(0).toInt(), 1);
    QCOMPARE(notifications.keys(), QList<int>{published.at(2).at(0).toInt()});
    // Nothing left to withdraw.
    notifications.receive(1, relayed({{QStringLiteral("type"), QStringLiteral("unload")}}));
    QCOMPARE(withdrawn.count(), 1);

    // The tab's view going is its page going.
    notifications.forgetTab(2);
    QCOMPARE(closed.count(), 3);
    QVERIFY(notifications.keys().isEmpty());
}

void tst_webnotifications::tapAndDismiss()
{
    NotificationPermissions permissions;
    permissions.setAllowed(Chat, true);
    WebNotifications notifications(&permissions, QString());
    QSignalSpy published(&notifications, &WebNotifications::publishRequested);
    QSignalSpy closed(&notifications, &WebNotifications::closeRequested);
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);
    QSignalSpy tabs(&notifications, &WebNotifications::tabRequested);

    notifications.receive(5, relayed(showMessage(1, QStringLiteral("a"))));
    notifications.receive(5, relayed(showMessage(2, QStringLiteral("b"))));
    const int first = published.at(0).at(0).toInt();
    const int second = published.at(1).at(0).toInt();

    // Tapped: the tab to the front, the page told, and the notification closed.
    notifications.activate(first);
    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.at(0).at(0).toInt(), 5);
    QCOMPARE(closed.count(), 1);
    QCOMPARE(closed.at(0).at(0).toInt(), first);
    // Swiped away: the page told.
    notifications.closed(second);
    QCOMPARE(closed.count(), 1);
    QCOMPARE(replies(pages, 5), (QStringList{QStringLiteral("show:1"), QStringLiteral("show:2"),
                                             QStringLiteral("click:1"), QStringLiteral("close:1"),
                                             QStringLiteral("close:2")}));
    // Gone is gone.
    notifications.activate(first);
    notifications.closed(second);
    QCOMPARE(tabs.count(), 1);
    QCOMPARE(pages.count(), 5);
}

void tst_webnotifications::closeAll()
{
    NotificationPermissions permissions;
    permissions.setAllowed(Chat, true);
    WebNotifications notifications(&permissions, QString());
    QSignalSpy closed(&notifications, &WebNotifications::closeRequested);
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);
    notifications.receive(1, relayed(showMessage(1, QStringLiteral("a"))));
    notifications.receive(2, relayed(showMessage(1, QStringLiteral("b"))));
    permissions.remove(Chat);
    notifications.receive(3, relayed(requestMessage(1)));
    notifications.closeAll();
    QCOMPARE(closed.count(), 2);
    QVERIFY(notifications.keys().isEmpty());
    // The browser is closing: nothing is asked any more, and no page told.
    notifications.answer(3, WebNotifications::Allow);
    QCOMPARE(pages.count(), 2);
    QCOMPARE(permissions.rowCount(), 0);
}

void tst_webnotifications::ignoresWhatItDoesNotKnow()
{
    NotificationPermissions permissions;
    permissions.setAllowed(Chat, true);
    WebNotifications notifications(&permissions, QString());
    QSignalSpy published(&notifications, &WebNotifications::publishRequested);
    QSignalSpy asked(&notifications, &WebNotifications::permissionRequested);
    QSignalSpy pages(&notifications, &WebNotifications::pageRequested);

    const QVariantMap show = showMessage(1, QStringLiteral("a"));
    // No tab; not a map; not JSON; not an object.
    notifications.receive(0, relayed(show));
    notifications.receive(1, QStringLiteral("show"));
    notifications.receive(1, QVariantMap{{QStringLiteral("origin"), Chat},
                                         {QStringLiteral("detail"), QStringLiteral("{show")}});
    notifications.receive(1, QVariantMap{{QStringLiteral("origin"), Chat},
                                         {QStringLiteral("detail"), QStringLiteral("[1]")}});
    // No site: a data: page, a page that is no page at all.
    notifications.receive(1, relayed(show, QStringLiteral("null")));
    notifications.receive(1, relayed(show, QString()));
    // A page not named as the page script names one, a number that is none.
    QVariantMap named = show;
    named.insert(QStringLiteral("page"), QStringLiteral("Not a name!"));
    notifications.receive(1, relayed(named));
    QVariantMap unnumbered = show;
    unnumbered.insert(QStringLiteral("id"), 1.5);
    notifications.receive(1, relayed(unnumbered));
    unnumbered.remove(QStringLiteral("id"));
    notifications.receive(1, relayed(unnumbered));
    // Something it does not know.
    QVariantMap other = show;
    other.insert(QStringLiteral("type"), QStringLiteral("vibrate"));
    notifications.receive(1, relayed(other));
    // Longer than a message may be.
    QVariantMap huge = relayed(show);
    huge.insert(QStringLiteral("detail"),
                QString(WebNotifications::MessageLimit + 1, QLatin1Char(' ')));
    notifications.receive(1, huge);
    QCOMPARE(published.count(), 0);
    QCOMPARE(asked.count(), 0);
    QCOMPARE(pages.count(), 0);

    // Answers and taps nothing waits for.
    notifications.answer(9, WebNotifications::Allow);
    notifications.activate(42);
    notifications.closed(42);
    QCOMPARE(pages.count(), 0);
}

void tst_webnotifications::icons()
{
    QTemporaryDir dir;
    const QString icons = dir.path() + QStringLiteral("/notifications");
    // What a browser that stopped left behind goes as the next one starts; nothing else.
    QVERIFY(QDir().mkpath(icons));
    QFile left(icons + QStringLiteral("/1-1.png"));
    QVERIFY(left.open(QIODevice::WriteOnly));
    left.close();
    QFile other(icons + QStringLiteral("/keep.txt"));
    QVERIFY(other.open(QIODevice::WriteOnly));
    other.close();

    NotificationPermissions permissions;
    permissions.setAllowed(Chat, true);
    WebNotifications notifications(&permissions, icons);
    QVERIFY(!QFile::exists(left.fileName()));
    QVERIFY(QFile::exists(other.fileName()));
    QSignalSpy published(&notifications, &WebNotifications::publishRequested);
    const auto iconOf = [&published](int index) {
        return published.at(index).at(1).toMap().value(QStringLiteral("icon")).toString();
    };

    // The page's picture, as a file of the browser's cache, no larger than the platform
    // shows one.
    notifications.receive(
        1, relayed(showMessage(1, QStringLiteral("a"), QStringLiteral("t"), pngDataUrl(600, 300))));
    const QString first = iconOf(0);
    QVERIFY(!first.isEmpty());
    QCOMPARE(QFileInfo(first).absolutePath(), QDir(icons).absolutePath());
    QImage read(first);
    QCOMPARE(read.size(), QSize(WebNotifications::IconSize, WebNotifications::IconSize / 2));

    // Shown in its place: a file of its own, the last one gone.
    notifications.receive(
        1, relayed(showMessage(2, QStringLiteral("b"), QStringLiteral("t"), pngDataUrl(16, 16))));
    const QString second = iconOf(1);
    QVERIFY(!second.isEmpty());
    QVERIFY(second != first);
    QVERIFY(!QFile::exists(first));
    QCOMPARE(QImage(second).size(), QSize(16, 16));

    // Nothing but a PNG, and one that is one.
    notifications.receive(
        1, relayed(showMessage(3, QStringLiteral("c"), QString(),
                               QStringLiteral("data:image/svg+xml;base64,PHN2Zz4="))));
    notifications.receive(1, relayed(showMessage(4, QStringLiteral("d"), QString(),
                                                 QStringLiteral("data:image/png;base64,AAAA"))));
    notifications.receive(1, relayed(showMessage(5, QStringLiteral("e"), QString(),
                                                 QStringLiteral("https://chat.example/i.png"))));
    QCOMPARE(iconOf(2), QString());
    QCOMPARE(iconOf(3), QString());
    QCOMPARE(iconOf(4), QString());

    // Closed, its picture goes with it.
    notifications.closed(published.at(1).at(0).toInt());
    QVERIFY(!QFile::exists(second));

    // Without a directory to keep them in, there are no pictures.
    WebNotifications without(&permissions, QString());
    QSignalSpy withoutPublished(&without, &WebNotifications::publishRequested);
    without.receive(1, relayed(showMessage(1, QStringLiteral("a"), QString(), pngDataUrl(4, 4))));
    QCOMPARE(withoutPublished.at(0).at(1).toMap().value(QStringLiteral("icon")).toString(),
             QString());

    // A directory that cannot be made: none either.
    QFile blocker(dir.path() + QStringLiteral("/file"));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    WebNotifications blocked(&permissions, blocker.fileName() + QStringLiteral("/notifications"));
    QSignalSpy blockedPublished(&blocked, &WebNotifications::publishRequested);
    blocked.receive(1, relayed(showMessage(1, QStringLiteral("a"), QString(), pngDataUrl(4, 4))));
    QCOMPARE(blockedPublished.at(0).at(1).toMap().value(QStringLiteral("icon")).toString(),
             QString());
}

void tst_webnotifications::popupOpening()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QSignalSpy engine(&permissions, &NotificationPermissions::engineRequest);
    const QVariantMap refused{
        {QStringLiteral("title"), QStringLiteral("desktopNotification")},
        {QStringLiteral("host"), QStringLiteral("chat.example")},
        {QStringLiteral("id"), QStringLiteral("chat.example desktop-notification")}};

    // Not a permission, another permission, another host, a page that is no site.
    notifications.popupOpening(QStringLiteral("https://chat.example/"),
                               QStringLiteral("embed:alert"), refused);
    QVariantMap location = refused;
    location.insert(QStringLiteral("title"), QStringLiteral("geolocation"));
    notifications.popupOpening(QStringLiteral("https://chat.example/"),
                               QStringLiteral("embed:permissions"), location);
    notifications.popupOpening(QStringLiteral("https://other.example/"),
                               QStringLiteral("embed:permissions"), refused);
    notifications.popupOpening(QStringLiteral("about:blank"), QStringLiteral("embed:permissions"),
                               refused);
    QCoreApplication::processEvents();
    QCOMPARE(engine.count(), 0);

    // Taken back after the platform has answered, which it does as this returns.
    notifications.popupOpening(QStringLiteral("https://chat.example/room"),
                               QStringLiteral("embed:permissions"), refused);
    QCOMPARE(engine.count(), 0);
    QTRY_COMPARE(engine.count(), 1);
    QCOMPARE(engine.at(0).at(1).toMap().value(QStringLiteral("msg")).toString(),
             QStringLiteral("remove"));
    QCOMPARE(engine.at(0).at(1).toMap().value(QStringLiteral("uri")).toString(), Chat);
}

// The frame script, in a world made of the parts of the message manager it uses.
void tst_webnotifications::relayScript()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QJSEngine engine;
    QVERIFY(!engine.evaluate(QString::fromUtf8(FakeFrame)).isError());
    const QJSValue loaded = engine.evaluate(notifications.relayScript());
    QVERIFY2(!loaded.isError(), qPrintable(loaded.toString()));
    const auto js = [&engine](const QString &code) { return engine.evaluate(code); };

    // Listening in the capture phase, for what the page dispatches itself.
    QVERIFY(js(QStringLiteral("listeners['salama-notification'].capture")).toBool());
    QVERIFY(js(QStringLiteral("listeners['salama-notification'].untrusted")).toBool());

    js(QStringLiteral("fire('salama-notification', content, '{\"type\":\"show\"}')"));
    QCOMPARE(js(QStringLiteral("sent.length")).toInt(), 1);
    QCOMPARE(js(QStringLiteral("sent[0].name")).toString(), notifications.messageName());
    QCOMPARE(js(QStringLiteral("sent[0].data.origin")).toString(), Chat);
    QCOMPARE(js(QStringLiteral("sent[0].data.permission")).toString(), QStringLiteral("granted"));
    QCOMPARE(js(QStringLiteral("sent[0].data.detail")).toString(),
             QStringLiteral("{\"type\":\"show\"}"));

    // A frame's, something but a string, more than a message may be: nothing.
    js(QStringLiteral("fire('salama-notification', { frame: true }, '{}')"));
    js(QStringLiteral("fire('salama-notification', content, { type: 'show' })"));
    js(QStringLiteral("fire('salama-notification', content, new Array(%1).join('x'))")
           .arg(WebNotifications::MessageLimit + 2));
    QCOMPARE(js(QStringLiteral("sent.length")).toInt(), 1);

    // The permission as Gecko has it, or nothing when it cannot be read.
    js(QStringLiteral("nativePermission = null;"
                      "fire('salama-notification', content, '{}')"));
    QCOMPARE(js(QStringLiteral("sent[1].data.permission")).toString(), QString());

    // The document going, but not a frame's.
    js(QStringLiteral("fire('pagehide', { frame: true })"));
    QCOMPARE(js(QStringLiteral("sent.length")).toInt(), 2);
    js(QStringLiteral("fire('pagehide', content.document)"));
    QCOMPARE(js(QStringLiteral("sent.length")).toInt(), 3);
    QCOMPARE(js(QStringLiteral("sent[2].data.detail")).toString(),
             QStringLiteral("{\"type\":\"unload\"}"));
}

void tst_webnotifications::pageScriptInstallsOnce()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QJSEngine engine;
    QVERIFY(!engine.evaluate(QString::fromUtf8(FakePage)).isError());
    engine.globalObject().setProperty(QStringLiteral("script"), notifications.pageScript());
    const auto js = [&engine](const QString &code) { return engine.evaluate(code); };

    const QJSValue installed = js(QStringLiteral("install(script)"));
    QVERIFY2(installed.toBool(), qPrintable(installed.toString()));
    QVERIFY(js(QStringLiteral("window.Notification !== NativeNotification")).toBool());
    js(QStringLiteral("var first = window.Notification"));
    QVERIFY(js(QStringLiteral("install(script)")).toBool());
    QVERIFY(js(QStringLiteral("window.Notification === first")).toBool());
    QCOMPARE(js(QStringLiteral("window.Notification.name")).toString(),
             QStringLiteral("Notification"));
    QCOMPARE(js(QStringLiteral("Object.prototype.toString.call(new first('x'))")).toString(),
             QStringLiteral("[object Notification]"));

    // A page with no Notification of its own is left as it is.
    QJSEngine bare;
    bare.evaluate(QString::fromUtf8(FakePage));
    bare.evaluate(QStringLiteral("delete window.Notification"));
    bare.globalObject().setProperty(QStringLiteral("script"), notifications.pageScript());
    QVERIFY(!bare.evaluate(QStringLiteral("install(script)")).toBool());
    QVERIFY(bare.evaluate(QStringLiteral("window.Notification === undefined")).toBool());
}

void tst_webnotifications::pageScriptPermission()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QJSEngine engine;
    engine.evaluate(QString::fromUtf8(FakePage));
    engine.globalObject().setProperty(QStringLiteral("script"), notifications.pageScript());
    const auto js = [&engine](const QString &code) {
        const QJSValue value = engine.evaluate(code);
        if (value.isError()) {
            qWarning() << code << value.toString();
        }
        return value;
    };
    js(QStringLiteral("install(script); var N = window.Notification;"
                      "var results = [], called = [];"
                      "function ask() { N.requestPermission(function (p) { called.push(p); })"
                      "                  .then(function (p) { results.push(p); }); }"));

    // Gecko's permission, read as it is.
    QCOMPARE(js(QStringLiteral("N.permission")).toString(), QStringLiteral("default"));
    js(QStringLiteral("permission = 'denied'"));
    QCOMPARE(js(QStringLiteral("N.permission")).toString(), QStringLiteral("denied"));
    js(QStringLiteral("ask()"));
    QTRY_COMPARE(js(QStringLiteral("results.join()")).toString(), QStringLiteral("denied"));
    QCOMPARE(js(QStringLiteral("called.join()")).toString(), QStringLiteral("denied"));
    js(QStringLiteral("permission = 'default'; results = []; called = []"));

    // Not while the page handles a touch: not asked, and nothing decided.
    js(QStringLiteral("ask()"));
    QTRY_COMPARE(js(QStringLiteral("results.join()")).toString(), QStringLiteral("default"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 0);
    // A touch the page made up is no touch.
    js(QStringLiteral("window.dispatchEvent(new Event('touchend'));"
                      "results = []; ask()"));
    QTRY_COMPARE(js(QStringLiteral("results.join()")).toString(), QStringLiteral("default"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 0);

    // Asked as a touch is handled; allowed.
    js(QStringLiteral("results = []; called = []; touch(); ask()"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 1);
    QCOMPARE(js(QStringLiteral("last().type")).toString(), QStringLiteral("request"));
    const int id = js(QStringLiteral("last().id")).toInt();
    const QString page = js(QStringLiteral("last().page")).toString();
    QVERIFY(QRegularExpression(QStringLiteral("^[a-z0-9]{1,32}$")).match(page).hasMatch());
    // An answer for another page is not this one's.
    const auto answer = [&](int requestId, const QString &state, const QString &to) {
        engine.globalObject().setProperty(
            QStringLiteral("answer"),
            WebNotifications::replyScript({{QStringLiteral("type"), QStringLiteral("permission")},
                                           {QStringLiteral("id"), requestId},
                                           {QStringLiteral("permission"), state},
                                           {QStringLiteral("page"), to}}));
        js(QStringLiteral("reply(answer)"));
    };
    answer(id, QStringLiteral("granted"), QStringLiteral("elsewhere"));
    QCoreApplication::processEvents();
    QCOMPARE(js(QStringLiteral("results.length")).toInt(), 0);
    answer(id, QStringLiteral("granted"), page);
    QTRY_COMPARE(js(QStringLiteral("results.join()")).toString(), QStringLiteral("granted"));
    QCOMPARE(js(QStringLiteral("called.join()")).toString(), QStringLiteral("granted"));
    // Granted, though the engine has yet to say so.
    QCOMPARE(js(QStringLiteral("N.permission")).toString(), QStringLiteral("granted"));
    // Answered once.
    answer(id, QStringLiteral("denied"), page);
    QCOMPARE(js(QStringLiteral("N.permission")).toString(), QStringLiteral("granted"));
}

// Refused, a page is not asked again until it is loaded again.
void tst_webnotifications::pageScriptRefusal()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QJSEngine other;
    other.evaluate(QString::fromUtf8(FakePage));
    other.globalObject().setProperty(QStringLiteral("script"), notifications.pageScript());
    other.evaluate(
        QStringLiteral("install(script); var N = window.Notification, results = [];"
                       "touch(); N.requestPermission().then(function (p) { results.push(p); });"));
    other.globalObject().setProperty(
        QStringLiteral("answer"),
        WebNotifications::replyScript(
            {{QStringLiteral("type"), QStringLiteral("permission")},
             {QStringLiteral("id"), other.evaluate(QStringLiteral("last().id")).toInt()},
             {QStringLiteral("permission"), QStringLiteral("denied")},
             {QStringLiteral("page"), other.evaluate(QStringLiteral("last().page")).toString()}}));
    other.evaluate(QStringLiteral("reply(answer)"));
    QTRY_COMPARE(other.evaluate(QStringLiteral("results.join()")).toString(),
                 QStringLiteral("denied"));
    other.evaluate(
        QStringLiteral("touch(); N.requestPermission().then(function (p) { results.push(p); });"));
    QTRY_COMPARE(other.evaluate(QStringLiteral("results.join()")).toString(),
                 QStringLiteral("denied,denied"));
    QCOMPARE(other.evaluate(QStringLiteral("posted.length")).toInt(), 1);
    // A callback that throws is the page's error, not the promise's.
    other.evaluate(QStringLiteral("permission = 'granted';"
                                  "N.requestPermission(function () { throw new Error('page'); })"
                                  "  .then(function (p) { results.push(p); });"));
    QTRY_COMPARE(other.evaluate(QStringLiteral("results.join()")).toString(),
                 QStringLiteral("denied,denied,granted"));
    QCOMPARE(other.evaluate(QStringLiteral("timers.length")).toInt(), 1);
}

void tst_webnotifications::pageScriptNotifications()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QJSEngine engine;
    engine.evaluate(QString::fromUtf8(FakePage));
    engine.globalObject().setProperty(QStringLiteral("script"), notifications.pageScript());
    const auto js = [&engine](const QString &code) {
        const QJSValue value = engine.evaluate(code);
        if (value.isError()) {
            qWarning() << code << value.toString();
        }
        return value;
    };
    const auto say = [&](const QString &type, int id) {
        engine.globalObject().setProperty(
            QStringLiteral("answer"),
            WebNotifications::replyScript(
                {{QStringLiteral("type"), type},
                 {QStringLiteral("id"), id},
                 {QStringLiteral("page"), js(QStringLiteral("posted[0].page")).toString()}}));
        js(QStringLiteral("reply(answer)"));
    };
    js(QStringLiteral(
        "install(script); var N = window.Notification, events = [];"
        "function watch(n, name) {"
        "  ['show', 'click', 'close', 'error'].forEach(function (type) {"
        "    n.addEventListener(type, function (e) { events.push(name + ':' + type); });"
        "  });"
        "}"));

    // As Gecko's constructor is: a title at least, a direction it knows.
    QVERIFY(
        js(QStringLiteral(
               "(function () { try { new N(); } catch (e) { return e instanceof TypeError; } })()"))
            .toBool());
    QVERIFY(js(QStringLiteral("(function () { try { new N('x', { dir: 'up' }); } catch (e) { "
                              "return e instanceof TypeError; } })()"))
                .toBool());
    QVERIFY(js(QStringLiteral("(function () { try { new N('x', 5); } catch (e) { return e "
                              "instanceof TypeError; } })()"))
                .toBool());

    // Not allowed: an error, and nothing said to the browser.
    js(QStringLiteral("var refused = new N('no'); watch(refused, 'refused');"
                      "var fired = []; refused.onerror = function () { fired.push('onerror'); };"));
    QCOMPARE(js(QStringLiteral("events.length")).toInt(), 0);
    js(QStringLiteral("runTimers()"));
    QCOMPARE(js(QStringLiteral("events.join()")).toString(), QStringLiteral("refused:error"));
    QCOMPARE(js(QStringLiteral("fired.join()")).toString(), QStringLiteral("onerror"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 0);
    // Closed already.
    js(QStringLiteral("refused.close()"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 0);

    // Allowed: what it holds, and the browser told.
    js(QStringLiteral("permission = 'granted'; events = [];"
                      "var n = new N('Hello', { body: 'World', tag: 'room', lang: 'fi',"
                      "  dir: 'ltr', data: { a: [1] }, requireInteraction: 1, silent: true });"
                      "watch(n, 'n');"));
    QCOMPARE(
        js(QStringLiteral("[n.title, n.body, n.tag, n.lang, n.dir, n.icon].join('|')")).toString(),
        QStringLiteral("Hello|World|room|fi|ltr|"));
    QCOMPARE(js(QStringLiteral("JSON.stringify(n.data)")).toString(),
             QStringLiteral("{\"a\":[1]}"));
    QVERIFY(js(QStringLiteral("n.requireInteraction === true && n.silent === true")).toBool());
    QVERIFY(js(QStringLiteral("new N('x').data === null && new N('x').dir === 'auto'")).toBool());
    QCOMPARE(js(QStringLiteral("posted[0].type")).toString(), QStringLiteral("show"));
    QCOMPARE(js(QStringLiteral("posted[0].title + '/' + posted[0].body + '/' + posted[0].tag"))
                 .toString(),
             QStringLiteral("Hello/World/room"));
    QVERIFY(js(QStringLiteral("posted[0].icon === undefined")).toBool());
    const int id = js(QStringLiteral("posted[0].id")).toInt();

    // What the browser says, in order; the handler properties as Gecko's are.
    js(QStringLiteral(
        "n.onclick = function (e) { events.push('onclick:' + (this === n)); return false; };"
        "n.onclick = n.onclick; var prevented = null;"
        "n.addEventListener('click', function (e) { prevented = e.defaultPrevented; });"));
    QVERIFY(js(QStringLiteral("typeof n.onclick === 'function' && n.onshow === null")).toBool());
    say(QStringLiteral("show"), id);
    say(QStringLiteral("click"), id);
    QCOMPARE(js(QStringLiteral("events.join()")).toString(),
             QStringLiteral("n:show,n:click,onclick:true"));
    QVERIFY(js(QStringLiteral("prevented")).toBool());
    js(QStringLiteral("n.onclick = 'not a function'"));
    QVERIFY(js(QStringLiteral("n.onclick === null")).toBool());

    // Another number, another page's: nothing.
    say(QStringLiteral("click"), id + 100);
    engine.globalObject().setProperty(
        QStringLiteral("answer"),
        WebNotifications::replyScript({{QStringLiteral("type"), QStringLiteral("click")},
                                       {QStringLiteral("id"), id},
                                       {QStringLiteral("page"), QStringLiteral("elsewhere")}}));
    js(QStringLiteral("reply(answer)"));
    js(QStringLiteral(
        "window.dispatchEvent(new CustomEvent('salama-notification-reply', { detail: '{' }))"));
    QCOMPARE(js(QStringLiteral("events.length")).toInt(), 3);

    // Closed by the page: the browser told, and the page's close a moment later.
    js(QStringLiteral("events = []; n.close(); n.close()"));
    QCOMPARE(js(QStringLiteral("last().type + ':' + last().id")).toString(),
             QStringLiteral("close:%1").arg(id));
    QCOMPARE(js(QStringLiteral("posted.filter(function (m) { return m.type === 'close'; }).length"))
                 .toInt(),
             1);
    js(QStringLiteral("runTimers()"));
    QCOMPARE(js(QStringLiteral("events.join()")).toString(), QStringLiteral("n:close"));
    // What the browser says after is nothing to it.
    say(QStringLiteral("close"), id);
    QCOMPARE(js(QStringLiteral("events.join()")).toString(), QStringLiteral("n:close"));

    // Closed by the browser, or refused by it.
    js(QStringLiteral(
        "events = []; var m = new N('m'); watch(m, 'm'); var e = new N('e'); watch(e, 'e');"));
    const int mId = js(QStringLiteral("m ? posted[posted.length - 2].id : 0")).toInt();
    const int eId = js(QStringLiteral("last().id")).toInt();
    say(QStringLiteral("close"), mId);
    say(QStringLiteral("error"), eId);
    QCOMPARE(js(QStringLiteral("events.join()")).toString(), QStringLiteral("m:close,e:error"));
    const int before = js(QStringLiteral("posted.length")).toInt();
    js(QStringLiteral("m.close()"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), before);

    // Data that cannot be cloned is the page's error, as Gecko's is.
    QVERIFY(js(QStringLiteral("(function () { try { new N('x', { data: function () {} }); } catch "
                              "(e) { return true; } return false; })()"))
                .toBool());
    QCOMPARE(js(QStringLiteral("errors.length")).toInt(), 0);
}

void tst_webnotifications::pageScriptIcons()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QJSEngine engine;
    engine.evaluate(QString::fromUtf8(FakePage));
    engine.globalObject().setProperty(QStringLiteral("script"), notifications.pageScript());
    const auto js = [&engine](const QString &code) {
        const QJSValue value = engine.evaluate(code);
        if (value.isError()) {
            qWarning() << code << value.toString();
        }
        return value;
    };
    js(QStringLiteral("install(script); var N = window.Notification; permission = 'granted';"));

    // Its address as the page's, and nothing said until it has loaded, drawn no larger
    // than the platform shows one.
    js(QStringLiteral("var a = new N('a', { icon: 'avatar.png' })"));
    QCOMPARE(js(QStringLiteral("a.icon")).toString(),
             QStringLiteral("https://chat.example/room/avatar.png"));
    QCOMPARE(js(QStringLiteral("images[0].src")).toString(),
             QStringLiteral("https://chat.example/room/avatar.png"));
    QCOMPARE(js(QStringLiteral("images[0].crossOrigin")).toString(), QStringLiteral("anonymous"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 0);
    QCOMPARE(js(QStringLiteral("timers[0].delay")).toInt(), 3000);
    js(QStringLiteral(
        "images[0].naturalWidth = 512; images[0].naturalHeight = 1024; images[0].onload()"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 1);
    QCOMPARE(js(QStringLiteral("drawn.join()")).toString(), QStringLiteral("128x256"));
    QCOMPARE(js(QStringLiteral("posted[0].icon")).toString(),
             QStringLiteral("data:image/png;base64,128x256"));
    // The wait running out after changes nothing.
    js(QStringLiteral("runTimers()"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 1);

    // Small ones as they are; one with no size of its own, as large as they are shown.
    js(QStringLiteral(
        "drawn = []; new N('b', { icon: '/b.png' });"
        "images[1].naturalWidth = 32; images[1].naturalHeight = 16; images[1].onload();"
        "new N('c', { icon: 'data:image/svg+xml,x' });"
        "images[2].naturalWidth = 0; images[2].naturalHeight = 0; images[2].onload();"));
    QCOMPARE(js(QStringLiteral("images[1].src")).toString(),
             QStringLiteral("https://chat.example/b.png"));
    QCOMPARE(js(QStringLiteral("drawn.join()")).toString(), QStringLiteral("32x16,256x256"));

    // One that will not load, will not be drawn -- from another site that does not
    // allow it -- or takes too long, and it is shown without.
    js(QStringLiteral("timers = []; posted = [];"
                      "new N('d', { icon: 'd.png' }); images[3].onerror();"
                      "tainted = true; new N('e', { icon: 'e.png' });"
                      "images[4].naturalWidth = 8; images[4].naturalHeight = 8; images[4].onload();"
                      "tainted = false; new N('f', { icon: 'f.png' });"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 2);
    js(QStringLiteral("runTimers()"));
    QCOMPARE(js(QStringLiteral(
                    "posted.map(function (m) { return m.title + ':' + (m.icon || ''); }).join()"))
                 .toString(),
             QStringLiteral("d:,e:,f:"));

    // An address that is none is no icon; one closed before its icon came is not shown.
    js(QStringLiteral("posted = []; var g = new N('g', { icon: 'http://[bad' });"));
    QCOMPARE(js(QStringLiteral("g.icon")).toString(), QString());
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 1);
    js(QStringLiteral("var h = new N('h', { icon: 'h.png' }); h.close(); runTimers();"));
    QCOMPARE(js(QStringLiteral("posted.map(function (m) { return m.type; }).join()")).toString(),
             QStringLiteral("show,close"));
}

void tst_webnotifications::pageScriptServiceWorker()
{
    NotificationPermissions permissions;
    WebNotifications notifications(&permissions, QString());
    QJSEngine engine;
    engine.evaluate(QString::fromUtf8(FakePage));
    engine.globalObject().setProperty(QStringLiteral("script"), notifications.pageScript());
    const auto js = [&engine](const QString &code) {
        const QJSValue value = engine.evaluate(code);
        if (value.isError()) {
            qWarning() << code << value.toString();
        }
        return value;
    };
    js(QStringLiteral(
        "install(script); var results = [];"
        "var registration = new ServiceWorkerRegistration('https://chat.example/');"
        "var elsewhere = new ServiceWorkerRegistration('https://chat.example/other/');"
        "function note(p) { p.then(function (v) { results.push(v === undefined ? 'shown' : v); },"
        "                         function (e) { results.push(e.name || String(e)); }); }"));

    // Refused without the permission, or without a title.
    js(QStringLiteral("note(registration.showNotification('a'))"));
    QTRY_COMPARE(js(QStringLiteral("results.join()")).toString(), QStringLiteral("TypeError"));
    js(QStringLiteral("permission = 'granted'; results = [];"
                      "note(registration.showNotification())"));
    QTRY_COMPARE(js(QStringLiteral("results.join()")).toString(), QStringLiteral("TypeError"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 0);

    js(QStringLiteral("results = [];"
                      "note(registration.showNotification('a', { tag: 'x' }));"
                      "note(registration.showNotification('b'));"
                      "note(elsewhere.showNotification('c'));"));
    QTRY_COMPARE(js(QStringLiteral("results.join()")).toString(),
                 QStringLiteral("shown,shown,shown"));
    QCOMPARE(js(QStringLiteral("posted.length")).toInt(), 3);

    // What the registration shows, by tag, as the same objects each time.
    js(QStringLiteral(
        "var all = null, tagged = null, again = null;"
        "registration.getNotifications().then(function (l) { all = l; });"
        "registration.getNotifications({ tag: 'x' }).then(function (l) { tagged = l; });"
        "registration.getNotifications().then(function (l) { again = l; });"));
    QTRY_VERIFY(js(QStringLiteral("again !== null")).toBool());
    QCOMPARE(js(QStringLiteral("all.map(function (n) { return n.title; }).join()")).toString(),
             QStringLiteral("a,b"));
    QCOMPARE(js(QStringLiteral("tagged.map(function (n) { return n.title; }).join()")).toString(),
             QStringLiteral("a"));
    QVERIFY(js(QStringLiteral("all[0] === again[0] && all[0] instanceof window.Notification"))
                .toBool());

    // A tap is the worker's, which nothing here reaches: nothing for the page. Closed,
    // it is no longer among them.
    js(QStringLiteral("var events = [];"
                      "all[0].addEventListener('click', function () { events.push('click'); });"
                      "all[0].addEventListener('close', function () { events.push('close'); });"));
    engine.globalObject().setProperty(
        QStringLiteral("answer"),
        WebNotifications::replyScript(
            {{QStringLiteral("type"), QStringLiteral("click")},
             {QStringLiteral("id"), js(QStringLiteral("posted[0].id")).toInt()},
             {QStringLiteral("page"), js(QStringLiteral("posted[0].page")).toString()}}));
    js(QStringLiteral("reply(answer)"));
    js(QStringLiteral("all[1].close(); runTimers();"
                      "registration.getNotifications().then(function (l) { again = l; })"));
    QTRY_COMPARE(js(QStringLiteral("again.length")).toInt(), 1);
    QCOMPARE(js(QStringLiteral("events.length")).toInt(), 0);
    QCOMPARE(js(QStringLiteral("last().type")).toString(), QStringLiteral("close"));
}

QTEST_GUILESS_MAIN(tst_webnotifications)
#include "tst_webnotifications.moc"
