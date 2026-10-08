// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "WebNotifications.h"

#include "NotificationPermissions.h"
#include "engine/EngineData.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QtDebug>
#include <algorithm>
#include <iterator>
#include <utility>

namespace Salama {

namespace {

const QString PageEvent = QStringLiteral("salama-notification");
const QString Message = QStringLiteral("salama:notification");
const QString ReplyEvent = QStringLiteral("salama-notification-reply");
const QString Mark = QStringLiteral("salamaNotifications");

// Ask only during user gesture (dom.webnotifications.requireuserinteraction), 5 s window
// (dom.user_activation.transient.timeout). navigator.userActivation needs Firefox 120, so
// page script tracks last engine-delivered touch/key itself.
const int ActivationMs = 5000;
const int IconWaitMs = 3000;

// Platform PopupOpener.qml refusal; title = ContentPermissionPrompt.js entity.
const QString PermissionsTopic = QStringLiteral("embed:permissions");
const QString PermissionTitle = QStringLiteral("desktopNotification");

const QString PngDataUrl = QStringLiteral("data:image/png;base64,");

// Own function: shares scope with embedlite helpers. Reads permission via native
// Notification, not page's replacement. Reports unload: Gecko closes notifications then.
const char *const RelayTemplate = R"((function () {
    "use strict";
    var pageEvent = "%1", message = "%2", limit = %3;
    function send(detail) {
        var origin = "", permission = "";
        try { origin = String(content.location.origin); } catch (e) {}
        try { permission = String(content.Notification.permission); } catch (e) {}
        sendAsyncMessage(message, { origin: origin, permission: permission, detail: detail });
    }
    addEventListener(pageEvent, function (event) {
        if (event.target === content && typeof event.detail === "string"
                && event.detail.length <= limit) {
            send(event.detail);
        }
    }, true, true);
    addEventListener("pagehide", function (event) {
        if (event.target === content.document) {
            send(JSON.stringify({ type: "unload" }));
        }
    }, true);
})();
)";

// Asks only during touch/key; refused -> no re-ask until reload. SW notifications get no
// events (notificationclick unreachable). Random document name: stale replies miss next page.
const char *const PageTemplate = R"( var EVENT = '%1', REPLY = '%2', MARK = '%3';
 var ICON = %4, WAIT = %5, ACTIVATION = %6;
 if (window[MARK]) { return true; }
 var Native = window.Notification;
 if (typeof Native !== 'function' || typeof EventTarget !== 'function') { return false; }
 Object.defineProperty(window, MARK, { value: true });
 var page = Math.random().toString(36).slice(2, 12) || 'page';
 var nextId = 1, fields = new WeakMap(), shown = {}, requests = {}, adopting = null;
 var granted = false, declined = false, lastInput = -Infinity;
 ['touchend', 'mousedown', 'pointerdown', 'keydown', 'click'].forEach(function (type) {
   window.addEventListener(type, function (event) {
     if (event.isTrusted) { lastInput = Date.now(); }
   }, true);
 });
 var post = function (message) {
   message.page = page;
   window.dispatchEvent(new CustomEvent(EVENT, { detail: JSON.stringify(message) }));
 };
 var later = function (callback) { setTimeout(callback, 0); };
 var permissionNow = function () {
   var state = Native.permission;
   return state === 'default' && granted ? 'granted' : state;
 };
 var fire = function (target, type) {
   target.dispatchEvent(new Event(type, { cancelable: type === 'click' }));
 };
 var text = function (value, fallback) { return value === undefined ? fallback : String(value); };
 var make = function (title, options) {
   if (options === undefined || options === null) { options = {}; }
   if (typeof options !== 'object' && typeof options !== 'function') {
     throw new TypeError("Notification: 'options' is not an object.");
   }
   var dir = text(options.dir, 'auto');
   if (dir !== 'auto' && dir !== 'ltr' && dir !== 'rtl') {
     throw new TypeError("Notification: '" + dir + "' is not a valid value for 'dir'.");
   }
   var icon = text(options.icon, '');
   if (icon !== '') {
     try { icon = new URL(icon, document.baseURI).href; } catch (e) { icon = ''; }
   }
   return {
     id: nextId++, title: String(title), dir: dir, lang: text(options.lang, ''),
     body: text(options.body, ''), tag: text(options.tag, ''), icon: icon,
     data: options.data === undefined ? null : structuredClone(options.data),
     requireInteraction: !!options.requireInteraction, silent: !!options.silent,
     persistent: false, scope: '', closed: false, object: null, handlers: {}
   };
 };
 var show = function (own) {
   shown[own.id] = own;
   var message = { type: 'show', id: own.id, title: own.title, body: own.body, tag: own.tag };
   var sent = false;
   var send = function (icon) {
     if (sent || own.closed) { return; }
     sent = true;
     if (icon) { message.icon = icon; }
     post(message);
   };
   if (own.icon === '') { send(''); return; }
   var image = new Image();
   image.crossOrigin = 'anonymous';
   image.onload = function () {
     try {
       var width = image.naturalWidth || ICON, height = image.naturalHeight || ICON;
       var scale = Math.min(1, ICON / Math.max(width, height));
       var canvas = document.createElement('canvas');
       canvas.width = Math.max(1, Math.round(width * scale));
       canvas.height = Math.max(1, Math.round(height * scale));
       canvas.getContext('2d').drawImage(image, 0, 0, canvas.width, canvas.height);
       send(canvas.toDataURL('image/png'));
     } catch (e) {
       send('');
     }
   };
   image.onerror = function () { send(''); };
   setTimeout(function () { send(''); }, WAIT);
   image.src = own.icon;
 };
 class Notification extends EventTarget {
   constructor(title, options) {
     var own = adopting;
     if (!own && arguments.length < 1) {
       throw new TypeError('Notification constructor: At least 1 argument required, but only 0 passed');
     }
     super();
     var adopted = own !== null;
     if (!adopted) { own = make(title, options); }
     own.object = this;
     fields.set(this, own);
     if (adopted) { return; }
     if (permissionNow() !== 'granted') {
       own.closed = true;
       var self = this;
       later(function () { fire(self, 'error'); });
       return;
     }
     show(own);
   }
   get title() { return fields.get(this).title; }
   get dir() { return fields.get(this).dir; }
   get lang() { return fields.get(this).lang; }
   get body() { return fields.get(this).body; }
   get tag() { return fields.get(this).tag; }
   get icon() { return fields.get(this).icon; }
   get data() { return fields.get(this).data; }
   get requireInteraction() { return fields.get(this).requireInteraction; }
   get silent() { return fields.get(this).silent; }
   close() {
     var own = fields.get(this);
     if (own.closed) { return; }
     own.closed = true;
     delete shown[own.id];
     post({ type: 'close', id: own.id });
     if (!own.persistent) {
       var self = this;
       later(function () { fire(self, 'close'); });
     }
   }
   static get permission() { return permissionNow(); }
   static requestPermission(callback) {
     return new Promise(function (resolve) {
       var settle = function (state) {
         if (typeof callback === 'function') {
           try { callback(state); } catch (e) { later(function () { throw e; }); }
         }
         resolve(state);
       };
       var state = permissionNow();
       if (state !== 'default') { settle(state); return; }
       if (declined) { settle('denied'); return; }
       if (Date.now() - lastInput > ACTIVATION) { settle('default'); return; }
       var id = nextId++;
       requests[id] = settle;
       post({ type: 'request', id: id });
     });
   }
 }
 ['click', 'show', 'error', 'close'].forEach(function (type) {
   Object.defineProperty(Notification.prototype, 'on' + type, {
     configurable: true, enumerable: true,
     get: function () {
       var handler = fields.get(this).handlers[type];
       return handler === undefined ? null : handler;
     },
     set: function (value) {
       var own = fields.get(this), self = this;
       if (!(type in own.handlers)) {
         this.addEventListener(type, function (event) {
           var handler = own.handlers[type];
           if (typeof handler === 'function' && handler.call(self, event) === false) {
             event.preventDefault();
           }
         });
       }
       own.handlers[type] = typeof value === 'function' ? value : null;
     }
   });
 });
 Object.defineProperty(Notification.prototype, Symbol.toStringTag,
                       { value: 'Notification', configurable: true });
 Object.defineProperty(window, 'Notification',
                       { value: Notification, writable: true, configurable: true });
 var Registration = window.ServiceWorkerRegistration;
 if (typeof Registration === 'function' && Registration.prototype.showNotification) {
   Object.defineProperty(Registration.prototype, 'showNotification', {
     configurable: true, writable: true,
     value: function showNotification(title, options) {
       var scope = this.scope, own;
       try {
         if (arguments.length < 1) {
           throw new TypeError('ServiceWorkerRegistration.showNotification: At least 1 argument required, but only 0 passed');
         }
         own = make(title, options);
       } catch (e) {
         return Promise.reject(e);
       }
       if (permissionNow() !== 'granted') {
         return Promise.reject(new TypeError('ServiceWorkerRegistration.showNotification: Permission to show Notification denied.'));
       }
       own.persistent = true;
       own.scope = scope;
       show(own);
       return Promise.resolve();
     }
   });
   Object.defineProperty(Registration.prototype, 'getNotifications', {
     configurable: true, writable: true,
     value: function getNotifications(filter) {
       var scope = this.scope;
       var tag = filter && filter.tag !== undefined ? String(filter.tag) : '';
       var list = [];
       Object.keys(shown).forEach(function (id) {
         var own = shown[id];
         if (own.persistent && own.scope === scope && (tag === '' || own.tag === tag)) {
           if (!own.object) {
             adopting = own;
             try { new Notification(); } finally { adopting = null; }
           }
           list.push(own.object);
         }
       });
       return Promise.resolve(list);
     }
   });
 }
 window.addEventListener(REPLY, function (event) {
   var message;
   try { message = JSON.parse(event.detail); } catch (e) { return; }
   if (!message || message.page !== page) { return; }
   if (message.type === 'permission') {
     var settle = requests[message.id];
     if (settle) {
       delete requests[message.id];
       if (message.permission === 'granted') { granted = true; } else { declined = true; }
       settle(message.permission === 'granted' ? 'granted' : 'denied');
     }
     return;
   }
   var own = shown[message.id];
   if (!own) { return; }
   if (message.type === 'close' || message.type === 'error') {
     own.closed = true;
     delete shown[message.id];
   }
   if (own.object && !own.persistent) { fire(own.object, message.type); }
 });
 return true;)";

const char *const ReplyTemplate =
    R"(window.dispatchEvent(new CustomEvent('%1', { detail: %2 })); return true;)";

bool isPageName(const QString &page)
{
    static const QRegularExpression name(QStringLiteral("^[a-z0-9]{1,32}$"));
    return name.match(page).hasMatch();
}

QString clipped(const QString &text)
{
    return text.size() > WebNotifications::TextLimit
               ? text.left(WebNotifications::TextLimit) + QStringLiteral(" …")
               : text;
}

// JSON string = valid JS literal since ES2019 (U+2028/9 included).
QString literal(const QString &text)
{
    const QString array =
        QString::fromUtf8(QJsonDocument(QJsonArray{text}).toJson(QJsonDocument::Compact));
    return array.mid(1, array.size() - 2);
}

} // namespace

WebNotifications::WebNotifications(NotificationPermissions *permissions, QString iconDirectory,
                                   QObject *parent)
    : QObject(parent)
    , m_permissions(permissions)
    , m_iconDirectory(std::move(iconDirectory))
{
    // Leftover icons from crashed run.
    if (!m_iconDirectory.isEmpty()) {
        QDir directory(m_iconDirectory);
        const QStringList left = directory.entryList({QStringLiteral("*.png")}, QDir::Files);
        for (const QString &name : left) {
            directory.remove(name);
        }
    }
}

QString WebNotifications::messageName() const
{
    return Message;
}

QString WebNotifications::relayScript() const
{
    return QString::fromUtf8(RelayTemplate).arg(PageEvent, Message).arg(MessageLimit);
}

QString WebNotifications::relayScriptUrl() const
{
    return QStringLiteral("data:application/javascript;charset=utf-8,") +
           QString::fromLatin1(QUrl::toPercentEncoding(relayScript()));
}

QString WebNotifications::pageScript() const
{
    return QString::fromUtf8(PageTemplate)
        .arg(PageEvent, ReplyEvent, Mark)
        .arg(IconSize)
        .arg(IconWaitMs)
        .arg(ActivationMs);
}

void WebNotifications::receive(int tabId, const QVariant &data)
{
    const QVariantMap relayed = data.toMap();
    const QString detail = relayed.value(QStringLiteral("detail")).toString();
    if (tabId <= 0 || detail.size() > MessageLimit) {
        return;
    }
    const QVariantMap message = QJsonDocument::fromJson(detail.toUtf8()).object().toVariantMap();
    const QString type = message.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("unload")) {
        unload(tabId);
        return;
    }
    const QString origin =
        NotificationPermissions::originOf(relayed.value(QStringLiteral("origin")).toString());
    const QString page = message.value(QStringLiteral("page")).toString();
    const int id = EngineData::id(message.value(QStringLiteral("id")));
    if (origin.isEmpty() || !isPageName(page) || id == 0) {
        return;
    }
    if (type == QLatin1String("request")) {
        request(tabId, page, id, origin);
    } else if (type == QLatin1String("show")) {
        // Gecko's answer, or our allow if engine not caught up; never overrides denied.
        const QString permission = relayed.value(QStringLiteral("permission")).toString();
        const bool allowed =
            permission == QLatin1String("granted") ||
            (permission != QLatin1String("denied") && m_permissions->isAllowed(origin));
        show(tabId, page, message, origin, allowed);
    } else if (type == QLatin1String("close")) {
        close(tabId, page, id);
    }
}

void WebNotifications::request(int tabId, const QString &page, int id, const QString &origin)
{
    if (m_permissions->isBlocked(origin) || m_permissions->isAllowed(origin)) {
        const bool allowed = m_permissions->isAllowed(origin);
        reply(tabId, page,
              {{QStringLiteral("type"), QStringLiteral("permission")},
               {QStringLiteral("id"), id},
               {QStringLiteral("permission"),
                allowed ? QStringLiteral("granted") : QStringLiteral("denied")}});
        return;
    }
    const bool waiting = std::any_of(m_requests.cbegin(), m_requests.cend(),
                                     [tabId](const Request &one) { return one.tabId == tabId; });
    m_requests.append({tabId, page, id, origin});
    if (!waiting) {
        emit permissionRequested(tabId, NotificationPermissions::hostOf(origin));
    }
}

void WebNotifications::answer(int tabId, int decision)
{
    QVector<Request> answered;
    const auto isTabs = [tabId](const Request &one) { return one.tabId == tabId; };
    std::copy_if(m_requests.cbegin(), m_requests.cend(), std::back_inserter(answered), isTabs);
    m_requests.erase(std::remove_if(m_requests.begin(), m_requests.end(), isTabs),
                     m_requests.end());
    if (answered.isEmpty()) {
        return;
    }
    if (decision == Allow || decision == Block) {
        m_permissions->setAllowed(answered.first().origin, decision == Allow);
    }
    const QString state = decision == Allow ? QStringLiteral("granted") : QStringLiteral("denied");
    for (const Request &one : answered) {
        reply(tabId, one.page,
              {{QStringLiteral("type"), QStringLiteral("permission")},
               {QStringLiteral("id"), one.id},
               {QStringLiteral("permission"), state}});
    }
}

void WebNotifications::show(int tabId, const QString &page, const QVariantMap &message,
                            const QString &origin, bool allowed)
{
    const int id = EngineData::id(message.value(QStringLiteral("id")));
    if (!allowed) {
        reply(tabId, page,
              {{QStringLiteral("type"), QStringLiteral("error")}, {QStringLiteral("id"), id}});
        return;
    }
    // Same origin+tag replaces previous silently (Gecko Notification::GetAlertName).
    const QString tag = message.value(QStringLiteral("tag")).toString().left(TextLimit);
    Shown shown;
    if (!tag.isEmpty()) {
        const QHash<int, Shown> &all = m_shown;
        for (const Shown &one : all) {
            if (one.origin == origin && one.tag == tag) {
                shown = one;
                break;
            }
        }
    }
    removeIcon(shown.icon);
    if (shown.key == 0) {
        shown.key = m_nextKey++;
    }
    shown.tabId = tabId;
    shown.page = page;
    shown.id = id;
    shown.origin = origin;
    shown.tag = tag;
    shown.icon = saveIcon(shown.key, message.value(QStringLiteral("icon")).toString());
    m_shown.insert(shown.key, shown);
    emit publishRequested(
        shown.key,
        {{QStringLiteral("summary"), clipped(message.value(QStringLiteral("title")).toString())},
         {QStringLiteral("body"), clipped(message.value(QStringLiteral("body")).toString())},
         {QStringLiteral("subText"), NotificationPermissions::hostOf(origin)},
         {QStringLiteral("icon"), shown.icon}});
    reply(tabId, page,
          {{QStringLiteral("type"), QStringLiteral("show")}, {QStringLiteral("id"), id}});
}

void WebNotifications::close(int tabId, const QString &page, int id)
{
    const QHash<int, Shown> &all = m_shown;
    for (const Shown &one : all) {
        if (one.tabId == tabId && one.page == page && one.id == id) {
            const int key = one.key;
            take(key);
            emit closeRequested(key);
            return;
        }
    }
}

void WebNotifications::unload(int tabId)
{
    QList<int> keys;
    const QHash<int, Shown> &all = m_shown;
    for (const Shown &one : all) {
        if (one.tabId == tabId) {
            keys.append(one.key);
        }
    }
    std::sort(keys.begin(), keys.end());
    for (int key : keys) {
        take(key);
        emit closeRequested(key);
    }
    const int waiting = m_requests.count();
    m_requests.erase(std::remove_if(m_requests.begin(), m_requests.end(),
                                    [tabId](const Request &one) { return one.tabId == tabId; }),
                     m_requests.end());
    if (m_requests.count() != waiting) {
        emit permissionWithdrawn(tabId);
    }
}

void WebNotifications::activate(int key)
{
    if (!m_shown.contains(key)) {
        return;
    }
    const Shown shown = take(key);
    emit tabRequested(shown.tabId);
    reply(shown.tabId, shown.page,
          {{QStringLiteral("type"), QStringLiteral("click")}, {QStringLiteral("id"), shown.id}});
    reply(shown.tabId, shown.page,
          {{QStringLiteral("type"), QStringLiteral("close")}, {QStringLiteral("id"), shown.id}});
    emit closeRequested(key);
}

void WebNotifications::closed(int key)
{
    if (!m_shown.contains(key)) {
        return;
    }
    const Shown shown = take(key);
    reply(shown.tabId, shown.page,
          {{QStringLiteral("type"), QStringLiteral("close")}, {QStringLiteral("id"), shown.id}});
}

void WebNotifications::forgetTab(int tabId)
{
    unload(tabId);
}

void WebNotifications::closeAll()
{
    for (int key : keys()) {
        take(key);
        emit closeRequested(key);
    }
    m_requests.clear();
}

void WebNotifications::popupOpening(const QString &pageUrl, const QString &topic,
                                    const QVariant &data)
{
    const QVariantMap request = data.toMap();
    if (topic != PermissionsTopic ||
        request.value(QStringLiteral("title")).toString() != PermissionTitle) {
        return;
    }
    // Platform gives host only: undo for top page's host, not cross-origin frames (Gecko
    // refuses those anyway).
    const QString origin = NotificationPermissions::originOf(pageUrl);
    const QString host = request.value(QStringLiteral("host")).toString().toLower();
    if (origin.isEmpty() || QUrl(origin).host(QUrl::FullyEncoded) != host) {
        return;
    }
    // Deferred: must follow platform's refusal, sent on return. Single channel keeps order.
    NotificationPermissions *permissions = m_permissions;
    QTimer::singleShot(0, permissions,
                       [permissions, origin]() { permissions->undoAutomaticDenial(origin); });
}

QString WebNotifications::replyScript(const QVariantMap &message)
{
    const QString json = QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(message)).toJson(QJsonDocument::Compact));
    return QString::fromUtf8(ReplyTemplate).arg(ReplyEvent, literal(json));
}

QList<int> WebNotifications::keys() const
{
    QList<int> list = m_shown.keys();
    std::sort(list.begin(), list.end());
    return list;
}

int WebNotifications::tabOf(int key) const
{
    return m_shown.value(key).tabId;
}

void WebNotifications::reply(int tabId, const QString &page, const QVariantMap &message)
{
    QVariantMap addressed = message;
    addressed.insert(QStringLiteral("page"), page);
    emit pageRequested(tabId, replyScript(addressed));
}

WebNotifications::Shown WebNotifications::take(int key)
{
    const Shown shown = m_shown.take(key);
    removeIcon(shown.icon);
    return shown;
}

QString WebNotifications::saveIcon(int key, const QString &dataUrl)
{
    if (m_iconDirectory.isEmpty() || !dataUrl.startsWith(PngDataUrl)) {
        return {};
    }
    QImage image;
    if (!image.loadFromData(QByteArray::fromBase64(dataUrl.mid(PngDataUrl.size()).toLatin1()),
                            "PNG")) {
        return {};
    }
    if (image.width() > IconSize || image.height() > IconSize) {
        image = image.scaled(IconSize, IconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (!QDir().mkpath(m_iconDirectory)) {
        qWarning() << "WebNotifications: cannot create icon directory" << m_iconDirectory;
        return {};
    }
    QString path =
        QDir(m_iconDirectory).filePath(QStringLiteral("%1-%2.png").arg(key).arg(m_nextIcon++));
    if (!image.save(path, "PNG")) {
        return {};
    }
    return path;
}

void WebNotifications::removeIcon(const QString &path) const
{
    if (path.isEmpty() || QFileInfo(path).absolutePath() != QDir(m_iconDirectory).absolutePath()) {
        return;
    }
    QFile::remove(path);
}

} // namespace Salama
