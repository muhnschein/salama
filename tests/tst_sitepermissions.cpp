// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Site permissions: model of engine's list, per-kind exceptions view, defaults, and two
// notification models kept in sync.
#include "Core.h"
#include "engine/EnginePermissions.h"
#include "notifications/NotificationPermissions.h"
#include "permissions/SiteExceptions.h"
#include "permissions/SitePermissions.h"
#include "settings/SettingsSections.h"
#include "settings/SitePermissionSettings.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Salama::Core;
using Salama::NotificationPermissions;
using Salama::SiteExceptions;
using Salama::SitePermissions;
using Salama::SitePermissionSettings;

namespace {

const QString Topic = QStringLiteral("embed:perms:all");
const QString News = QStringLiteral("https://news.example");
const QString Chat = QStringLiteral("https://chat.example");
const QString Maps = QStringLiteral("https://maps.example");

// ContentPermissionManager.js payload via qtmozembed: every number double.
QVariantMap permission(const QString &type, const QString &uri, int capability, int expireType = 0)
{
    return {
        {QStringLiteral("type"), type},
        {QStringLiteral("uri"), uri},
        {QStringLiteral("capability"), static_cast<double>(capability)},
        {QStringLiteral("expireType"), static_cast<double>(expireType)},
    };
}

QVariantMap sent(const QSignalSpy &requests, int index)
{
    return requests.at(index).at(1).toMap();
}

QString originAt(const QAbstractItemModel &model, int row)
{
    return model.data(model.index(row, 0), roleId(SitePermissions::Role::Origin)).toString();
}

} // namespace

class tst_sitepermissions : public QObject
{
    Q_OBJECT

private slots:
    void kindsAreTheEnginesNames();
    void listIsRead();
    void listIgnoresTheRest();
    void listAsAString();
    void askedBySiteAndKind();
    void settingTellsTheEngine();
    void aLocationHasTwoNames();
    void removing();
    void adoptingTellsNobody();
    void refreshAsksForTheList();
    void exceptionsOfOneKind();
    void exceptionsViewRefusesOtherModels();
    void notificationsAreKeptInStep();
    void defaults();
    void defaultsPersist();
    void askingEachTime();
};

void tst_sitepermissions::kindsAreTheEnginesNames()
{
    QCOMPARE(Salama::permissionTypesOf(SitePermissions::Notifications),
             QStringList{QStringLiteral("desktop-notification")});
    QCOMPARE(Salama::permissionTypesOf(SitePermissions::Popups),
             QStringList{QStringLiteral("popup")});
    QCOMPARE(Salama::permissionTypesOf(SitePermissions::Cookies),
             QStringList{QStringLiteral("cookie")});
    QCOMPARE(Salama::permissionTypesOf(SitePermissions::Camera),
             QStringList{QStringLiteral("camera")});
    QCOMPARE(Salama::permissionTypesOf(SitePermissions::Microphone),
             QStringList{QStringLiteral("microphone")});
    QCOMPARE(Salama::permissionTypesOf(SitePermissions::TrackingProtection),
             QStringList{QStringLiteral("trackingprotection")});
    QCOMPARE(Salama::permissionTypesOf(SitePermissions::Location),
             (QStringList{QStringLiteral("geolocation"), QStringLiteral("geo")}));
    QVERIFY(Salama::permissionTypesOf(99).isEmpty());
    QVERIFY(Salama::permissionTypesOf(-1).isEmpty());

    for (int kind = SitePermissions::Notifications; kind <= SitePermissions::TrackingProtection;
         ++kind) {
        for (const QString &type : Salama::permissionTypesOf(kind)) {
            QCOMPARE(Salama::permissionKindOf(type), kind);
        }
    }
    QCOMPARE(Salama::permissionKindOf(QStringLiteral("camera ")), -1);
    QCOMPARE(Salama::permissionKindOf(QStringLiteral("storage-access")), -1);

    SitePermissions permissions;
    const QHash<int, QByteArray> roles = permissions.roleNames();
    QCOMPARE(roles.value(roleId(SitePermissions::Role::Kind)), QByteArray("kind"));
    QCOMPARE(roles.value(roleId(SitePermissions::Role::Origin)), QByteArray("origin"));
    QCOMPARE(roles.value(roleId(SitePermissions::Role::Host)), QByteArray("host"));
    QCOMPARE(roles.value(roleId(SitePermissions::Role::Allowed)), QByteArray("allowed"));
    QCOMPARE(permissions.topic(), Topic);
}

void tst_sitepermissions::listIsRead()
{
    SitePermissions permissions;
    QSignalSpy changed(&permissions, &SitePermissions::changed);
    QVERIFY(permissions.exceptionSiteCount() == 0);

    permissions.observe(Topic, QVariantList{
                                   permission(QStringLiteral("popup"), News, 1),
                                   permission(QStringLiteral("popup"), Chat, 2),
                                   permission(QStringLiteral("cookie"), News, 2),
                                   permission(QStringLiteral("desktop-notification"), Chat, 1),
                                   permission(QStringLiteral("camera"), Maps, 1),
                                   permission(QStringLiteral("microphone"), Maps, 2),
                                   permission(QStringLiteral("geolocation"), Maps, 1),
                                   permission(QStringLiteral("trackingprotection"), News, 1),
                               });
    QCOMPARE(permissions.rowCount(), 8);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(permissions.revision(), 1);
    QCOMPARE(permissions.rowCount(permissions.index(0)), 0);

    const QModelIndex first = permissions.index(0);
    QCOMPARE(permissions.data(first, roleId(SitePermissions::Role::Kind)).toInt(),
             int(SitePermissions::Popups));
    QCOMPARE(permissions.data(first, roleId(SitePermissions::Role::Origin)).toString(), News);
    QCOMPARE(permissions.data(first, roleId(SitePermissions::Role::Host)).toString(),
             QStringLiteral("news.example"));
    QCOMPARE(permissions.data(first, roleId(SitePermissions::Role::Allowed)).toBool(), true);
    QVERIFY(
        !permissions.data(permissions.index(1), roleId(SitePermissions::Role::Allowed)).toBool());
    QVERIFY(permissions.data(permissions.index(99), roleId(SitePermissions::Role::Kind)).isNull());
    QVERIFY(permissions.data(first, Qt::DisplayRole).isNull());
}

void tst_sitepermissions::listIgnoresTheRest()
{
    SitePermissions permissions;
    permissions.observe(
        Topic,
        QVariantList{
            // Session-only; platform's own deny; capability neither allow nor deny (cookie "first
            // party only" 9, prompt 3); other kind; non-site page; tracking protection deny (Gecko
            // allow list ignores it).
            permission(QStringLiteral("popup"), News, 2, 1),
            permission(QStringLiteral("cookie"), News, 9),
            permission(QStringLiteral("popup"), Chat, 3),
            permission(QStringLiteral("storage-access"), News, 1),
            permission(QStringLiteral("popup"), QStringLiteral("file:///tmp/x"), 1),
            permission(QStringLiteral("popup"), QStringLiteral("about:blank"), 1),
            permission(QStringLiteral("trackingprotection"), Maps, 2),
            permission(QStringLiteral("camera"),
                       QStringLiteral("https://cam.example^userContextId=1"), 1),
            permission(QStringLiteral("popup"), QStringLiteral("http://Host.Example:8080"), 1),
            permission(QStringLiteral("microphone"), Chat, 1),
            permission(QStringLiteral("microphone"), Chat, 2),
            QVariantMap{{QStringLiteral("type"), QStringLiteral("cookie")},
                        {QStringLiteral("uri"), Chat},
                        {QStringLiteral("capability"), QStringLiteral("1")},
                        {QStringLiteral("expireType"), QStringLiteral("0")}},
            QVariant(),
        });
    QCOMPARE(permissions.rowCount(), 3);
    QCOMPARE(permissions.decision(SitePermissions::Camera, QStringLiteral("https://cam.example")),
             int(SitePermissions::Allow));
    QCOMPARE(
        permissions.decision(SitePermissions::Popups, QStringLiteral("http://host.example:8080")),
        int(SitePermissions::Allow));
    QCOMPARE(permissions.decision(SitePermissions::Microphone, Chat), int(SitePermissions::Allow));
    QCOMPARE(permissions.decision(SitePermissions::TrackingProtection, Maps),
             int(SitePermissions::Default));

    permissions.observe(QStringLiteral("embed:download"), QVariantList{});
    QCOMPARE(permissions.rowCount(), 3);
    permissions.observe(Topic, QVariantList{});
    QCOMPARE(permissions.rowCount(), 0);
}

void tst_sitepermissions::listAsAString()
{
    SitePermissions permissions;
    permissions.observe(Topic,
                        QStringLiteral("[{\"type\":\"popup\",\"uri\":\"https://news.example\","
                                       "\"capability\":2,\"expireType\":0}]"));
    QCOMPARE(permissions.rowCount(), 1);
    QCOMPARE(permissions.decision(SitePermissions::Popups, News), int(SitePermissions::Block));
    permissions.observe(Topic, QStringLiteral("not json"));
    QCOMPARE(permissions.rowCount(), 0);
}

void tst_sitepermissions::askedBySiteAndKind()
{
    SitePermissions permissions;
    permissions.observe(Topic, QVariantList{
                                   permission(QStringLiteral("popup"), News, 1),
                                   permission(QStringLiteral("cookie"), News, 2),
                                   permission(QStringLiteral("popup"), Chat, 2),
                                   permission(QStringLiteral("camera"), Maps, 1),
                                   permission(QStringLiteral("trackingprotection"), Maps, 1),
                               });
    QCOMPARE(permissions.exceptionSiteCount(), 3);

    QCOMPARE(permissions.count(SitePermissions::Popups), 2);
    QCOMPARE(permissions.count(SitePermissions::Cookies), 1);
    QCOMPARE(permissions.count(SitePermissions::Camera), 1);
    QCOMPARE(permissions.count(SitePermissions::TrackingProtection), 1);
    QCOMPARE(permissions.count(SitePermissions::Location), 0);
    QCOMPARE(permissions.count(SitePermissions::Notifications), 0);

    QCOMPARE(permissions.originCount(News), 2);
    QCOMPARE(permissions.originCount(QStringLiteral("https://news.example/today?x=1")), 2);
    QCOMPARE(permissions.originCount(Maps), 2);
    QCOMPARE(permissions.originCount(QStringLiteral("https://other.example")), 0);
    QCOMPARE(permissions.originCount(QStringLiteral("about:blank")), 0);
    QCOMPARE(permissions.originCount(QString()), 0);
    QCOMPARE(
        permissions.decision(SitePermissions::Popups, QStringLiteral("https://news.example/x")),
        int(SitePermissions::Allow));
    QCOMPARE(permissions.decision(SitePermissions::Cookies, News), int(SitePermissions::Block));
    QCOMPARE(permissions.decision(SitePermissions::Popups, Chat), int(SitePermissions::Block));
    QCOMPARE(permissions.decision(SitePermissions::Camera, News), int(SitePermissions::Default));
    QCOMPARE(permissions.decision(SitePermissions::Popups, QStringLiteral("data:text/html,x")),
             int(SitePermissions::Default));
    // Other scheme or port = other site (origin).
    QCOMPARE(permissions.decision(SitePermissions::Popups, QStringLiteral("http://news.example")),
             int(SitePermissions::Default));
    QCOMPARE(
        permissions.decision(SitePermissions::Popups, QStringLiteral("https://news.example:8443")),
        int(SitePermissions::Default));

    QCOMPARE(SitePermissions::originOf(QStringLiteral("https://News.Example:443/a")), News);
    QVERIFY(SitePermissions::originOf(QStringLiteral("ftp://news.example")).isEmpty());
    QCOMPARE(Salama::EnginePermissions::hostOf(News), QStringLiteral("news.example"));
}

// Every change = engine message as ContentPermissionManager.js reads it; list follows.
void tst_sitepermissions::settingTellsTheEngine()
{
    SitePermissions permissions;
    QSignalSpy requests(&permissions, &SitePermissions::engineRequest);
    QSignalSpy changed(&permissions, &SitePermissions::changed);
    QSignalSpy decided(&permissions, &SitePermissions::decided);
    QSignalSpy inserted(&permissions, &QAbstractItemModel::rowsInserted);
    QSignalSpy data(&permissions, &QAbstractItemModel::dataChanged);

    permissions.set(SitePermissions::Popups, QStringLiteral("https://news.example/today"),
                    SitePermissions::Allow);
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.at(0).at(0).toString(), QStringLiteral("embedui:perms"));
    QCOMPARE(sent(requests, 0).value(QStringLiteral("msg")).toString(), QStringLiteral("add"));
    QCOMPARE(sent(requests, 0).value(QStringLiteral("uri")).toString(), News);
    QCOMPARE(sent(requests, 0).value(QStringLiteral("type")).toString(), QStringLiteral("popup"));
    QCOMPARE(sent(requests, 0).value(QStringLiteral("permission")).toInt(), 1);
    QCOMPARE(sent(requests, 0).value(QStringLiteral("expireType")).toInt(), 0);
    QCOMPARE(permissions.decision(SitePermissions::Popups, News), int(SitePermissions::Allow));
    QCOMPARE(permissions.rowCount(), 1);
    QCOMPARE(inserted.count(), 1);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(decided.count(), 1);
    QCOMPARE(decided.at(0).at(0).toInt(), int(SitePermissions::Popups));
    QCOMPARE(decided.at(0).at(1).toString(), News);
    QCOMPARE(decided.at(0).at(2).toInt(), int(SitePermissions::Allow));

    permissions.set(SitePermissions::Popups, News, SitePermissions::Block);
    QCOMPARE(sent(requests, 1).value(QStringLiteral("permission")).toInt(), 2);
    QCOMPARE(permissions.rowCount(), 1);
    QCOMPARE(data.count(), 1);
    QCOMPARE(permissions.decision(SitePermissions::Popups, News), int(SitePermissions::Block));
    QCOMPARE(changed.count(), 2);
    QCOMPARE(permissions.revision(), 2);

    // Repeat: no row change or changed signal, but engine told again (may have lost it).
    permissions.set(SitePermissions::Popups, News, SitePermissions::Block);
    QCOMPARE(requests.count(), 3);
    QCOMPARE(data.count(), 1);
    QCOMPARE(changed.count(), 2);

    permissions.set(SitePermissions::Cookies, News, SitePermissions::Allow);
    QCOMPARE(sent(requests, 3).value(QStringLiteral("type")).toString(), QStringLiteral("cookie"));
    QCOMPARE(permissions.rowCount(), 2);
    QCOMPARE(permissions.exceptionSiteCount(), 1);
    QCOMPARE(permissions.originCount(News), 2);

    permissions.set(SitePermissions::Popups, News, SitePermissions::Default);
    QCOMPARE(sent(requests, 4).value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(permissions.decision(SitePermissions::Popups, News), int(SitePermissions::Default));
    QCOMPARE(decided.last().at(2).toInt(), int(SitePermissions::Default));

    const int before = requests.count();
    permissions.set(SitePermissions::Popups, QStringLiteral("about:blank"), SitePermissions::Allow);
    permissions.set(SitePermissions::Popups, QString(), SitePermissions::Allow);
    permissions.set(99, News, SitePermissions::Allow);
    permissions.remove(99, News);
    permissions.remove(SitePermissions::Popups, QStringLiteral("ftp://x"));
    QCOMPARE(requests.count(), before);
    QCOMPARE(permissions.rowCount(), 1);
}

// Platform prompt writes "geolocation", Gecko uses "geo": write both, read either, so
// whichever engine uses exists.
void tst_sitepermissions::aLocationHasTwoNames()
{
    SitePermissions permissions;
    QSignalSpy requests(&permissions, &SitePermissions::engineRequest);
    permissions.set(SitePermissions::Location, Maps, SitePermissions::Block);
    QCOMPARE(requests.count(), 2);
    QCOMPARE(sent(requests, 0).value(QStringLiteral("type")).toString(),
             QStringLiteral("geolocation"));
    QCOMPARE(sent(requests, 1).value(QStringLiteral("type")).toString(), QStringLiteral("geo"));
    QCOMPARE(sent(requests, 1).value(QStringLiteral("permission")).toInt(), 2);
    QCOMPARE(permissions.rowCount(), 1);

    permissions.remove(SitePermissions::Location, Maps);
    QCOMPARE(requests.count(), 4);
    QCOMPARE(sent(requests, 2).value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(sent(requests, 2).value(QStringLiteral("type")).toString(),
             QStringLiteral("geolocation"));
    QCOMPARE(sent(requests, 3).value(QStringLiteral("type")).toString(), QStringLiteral("geo"));

    permissions.observe(Topic, QVariantList{permission(QStringLiteral("geo"), Maps, 1),
                                            permission(QStringLiteral("geolocation"), Maps, 1),
                                            permission(QStringLiteral("geolocation"), News, 2)});
    QCOMPARE(permissions.rowCount(), 2);
    QCOMPARE(permissions.count(SitePermissions::Location), 2);
    QCOMPARE(permissions.decision(SitePermissions::Location, Maps), int(SitePermissions::Allow));
}

void tst_sitepermissions::removing()
{
    SitePermissions permissions;
    permissions.observe(Topic, QVariantList{
                                   permission(QStringLiteral("popup"), News, 1),
                                   permission(QStringLiteral("popup"), Chat, 2),
                                   permission(QStringLiteral("cookie"), News, 2),
                                   permission(QStringLiteral("camera"), Chat, 1),
                               });
    QSignalSpy requests(&permissions, &SitePermissions::engineRequest);
    QSignalSpy removed(&permissions, &QAbstractItemModel::rowsRemoved);
    QSignalSpy decided(&permissions, &SitePermissions::decided);

    permissions.remove(SitePermissions::Popups, QStringLiteral("https://news.example/x"));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(sent(requests, 0).value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));
    QCOMPARE(sent(requests, 0).value(QStringLiteral("uri")).toString(), News);
    QCOMPARE(sent(requests, 0).value(QStringLiteral("type")).toString(), QStringLiteral("popup"));
    QCOMPARE(removed.count(), 1);
    QCOMPARE(permissions.rowCount(), 3);
    QCOMPARE(permissions.decision(SitePermissions::Cookies, News), int(SitePermissions::Block));
    QCOMPARE(decided.last().at(2).toInt(), int(SitePermissions::Default));

    permissions.remove(SitePermissions::Popups, News);
    QCOMPARE(requests.count(), 2);
    QCOMPARE(removed.count(), 1);

    permissions.observe(Topic, QVariantList{
                                   permission(QStringLiteral("popup"), News, 1),
                                   permission(QStringLiteral("popup"), Chat, 2),
                                   permission(QStringLiteral("cookie"), News, 2),
                                   permission(QStringLiteral("camera"), Chat, 1),
                               });
    permissions.removeAll(SitePermissions::Popups);
    QCOMPARE(requests.count(), 4);
    QCOMPARE(permissions.count(SitePermissions::Popups), 0);
    QCOMPARE(permissions.rowCount(), 2);
    QStringList removedSites{sent(requests, 2).value(QStringLiteral("uri")).toString(),
                             sent(requests, 3).value(QStringLiteral("uri")).toString()};
    removedSites.sort();
    QCOMPARE(removedSites, (QStringList{Chat, News}));
    permissions.removeAll(SitePermissions::Location);
    QCOMPARE(requests.count(), 4);

    permissions.observe(Topic, QVariantList{
                                   permission(QStringLiteral("popup"), News, 1),
                                   permission(QStringLiteral("cookie"), News, 2),
                                   permission(QStringLiteral("trackingprotection"), News, 1),
                                   permission(QStringLiteral("camera"), Chat, 1),
                               });
    QSignalSpy later(&permissions, &SitePermissions::engineRequest);
    permissions.removeAllForOrigin(QStringLiteral("https://news.example/page"));
    QCOMPARE(later.count(), 3);
    QStringList kinds;
    for (int i = 0; i < later.count(); ++i) {
        QCOMPARE(sent(later, i).value(QStringLiteral("uri")).toString(), News);
        kinds.append(sent(later, i).value(QStringLiteral("type")).toString());
    }
    kinds.sort();
    QCOMPARE(kinds, (QStringList{QStringLiteral("cookie"), QStringLiteral("popup"),
                                 QStringLiteral("trackingprotection")}));
    QCOMPARE(permissions.rowCount(), 1);
    QCOMPARE(permissions.exceptionSiteCount(), 1);
    QCOMPARE(permissions.originCount(News), 0);
    QCOMPARE(permissions.originCount(Chat), 1);
    permissions.removeAllForOrigin(QStringLiteral("about:blank"));
    QCOMPARE(later.count(), 3);
}

void tst_sitepermissions::adoptingTellsNobody()
{
    SitePermissions permissions;
    QSignalSpy requests(&permissions, &SitePermissions::engineRequest);
    QSignalSpy decided(&permissions, &SitePermissions::decided);
    QSignalSpy changed(&permissions, &SitePermissions::changed);

    permissions.adopt(SitePermissions::Notifications, Chat, SitePermissions::Allow);
    QCOMPARE(permissions.decision(SitePermissions::Notifications, Chat),
             int(SitePermissions::Allow));
    permissions.adopt(SitePermissions::Notifications, Chat, SitePermissions::Block);
    QCOMPARE(permissions.decision(SitePermissions::Notifications, Chat),
             int(SitePermissions::Block));
    QCOMPARE(changed.count(), 2);
    permissions.adopt(SitePermissions::Notifications, Chat, SitePermissions::Block);
    QCOMPARE(changed.count(), 2);
    permissions.adopt(SitePermissions::Notifications, Chat, SitePermissions::Default);
    QCOMPARE(permissions.rowCount(), 0);
    QCOMPARE(changed.count(), 3);
    permissions.adopt(SitePermissions::Notifications, Chat, SitePermissions::Default);
    permissions.adopt(SitePermissions::Notifications, QStringLiteral("about:blank"),
                      SitePermissions::Allow);
    permissions.adopt(99, Chat, SitePermissions::Allow);
    QCOMPARE(changed.count(), 3);
    QCOMPARE(requests.count(), 0);
    QCOMPARE(decided.count(), 0);
}

void tst_sitepermissions::refreshAsksForTheList()
{
    SitePermissions permissions;
    QSignalSpy requests(&permissions, &SitePermissions::engineRequest);
    permissions.refresh();
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.at(0).at(0).toString(), QStringLiteral("embedui:perms"));
    QCOMPARE(sent(requests, 0), (QVariantMap{{QStringLiteral("msg"), QStringLiteral("get-all")}}));
}

void tst_sitepermissions::exceptionsOfOneKind()
{
    SitePermissions permissions;
    SiteExceptions popups;
    QSignalSpy counts(&popups, &SiteExceptions::countChanged);
    QCOMPARE(popups.rowCount(), 0);
    popups.setKind(SitePermissions::Popups);
    popups.setPermissions(&permissions);
    QCOMPARE(popups.kind(), int(SitePermissions::Popups));
    QCOMPARE(popups.permissions(), static_cast<QObject *>(&permissions));
    QCOMPARE(popups.roleNames(), permissions.roleNames());

    permissions.observe(
        Topic, QVariantList{
                   permission(QStringLiteral("popup"), QStringLiteral("https://zed.example"), 1),
                   permission(QStringLiteral("popup"), Chat, 2),
                   permission(QStringLiteral("cookie"), Maps, 1),
                   permission(QStringLiteral("popup"), QStringLiteral("https://alpha.example"), 2),
                   permission(QStringLiteral("popup"), News, 1),
                   permission(QStringLiteral("popup"), QStringLiteral("http://news.example"), 1),
               });
    // Popups only; allowed first by host, then origin for same host (http before https);
    // then blocked by host.
    QCOMPARE(popups.rowCount(), 5);
    QCOMPARE(counts.count(), 1);
    QCOMPARE(popups.rowCount(), 5);
    const QStringList expected{QStringLiteral("http://news.example"), News,
                               QStringLiteral("https://zed.example"),
                               QStringLiteral("https://alpha.example"), Chat};
    for (int row = 0; row < expected.count(); ++row) {
        QCOMPARE(originAt(popups, row), expected.at(row));
    }
    QCOMPARE(popups.data(popups.index(0, 0), roleId(SitePermissions::Role::Allowed)).toBool(),
             true);
    QCOMPARE(popups.data(popups.index(4, 0), roleId(SitePermissions::Role::Allowed)).toBool(),
             false);

    QSignalSpy moved(&popups, &QAbstractItemModel::rowsMoved);
    QSignalSpy layout(&popups, &QAbstractItemModel::layoutChanged);
    permissions.set(SitePermissions::Popups, QStringLiteral("https://zed.example"),
                    SitePermissions::Block);
    QVERIFY(moved.count() + layout.count() > 0);
    QCOMPARE(originAt(popups, 2), QStringLiteral("https://alpha.example"));
    QCOMPARE(originAt(popups, 4), QStringLiteral("https://zed.example"));
    QCOMPARE(popups.rowCount(), 5);

    permissions.set(SitePermissions::Popups, Maps, SitePermissions::Allow);
    permissions.set(SitePermissions::Cookies, Chat, SitePermissions::Allow);
    QCOMPARE(popups.rowCount(), 6);
    QCOMPARE(originAt(popups, 0), Maps);
    permissions.remove(SitePermissions::Popups, Maps);
    QCOMPARE(popups.rowCount(), 5);
    QVERIFY(counts.count() >= 3);

    SiteExceptions cookies;
    cookies.setPermissions(&permissions);
    cookies.setKind(SitePermissions::Cookies);
    QCOMPARE(cookies.rowCount(), 2);
    QCOMPARE(cookies.rowCount(), 2);
    QSignalSpy kind(&cookies, &SiteExceptions::kindChanged);
    cookies.setKind(SitePermissions::Cookies);
    QCOMPARE(kind.count(), 0);
    cookies.setKind(SitePermissions::Camera);
    QCOMPARE(kind.count(), 1);
    QCOMPARE(cookies.rowCount(), 0);
    QCOMPARE(cookies.rowCount(), 0);
}

void tst_sitepermissions::exceptionsViewRefusesOtherModels()
{
    SitePermissions permissions;
    SiteExceptions view;
    QSignalSpy changed(&view, &SiteExceptions::permissionsChanged);
    QObject other;
    view.setPermissions(&other);
    view.setPermissions(nullptr);
    QVERIFY(view.permissions() == nullptr);
    QCOMPARE(changed.count(), 0);
    view.setPermissions(&permissions);
    view.setPermissions(&permissions);
    QCOMPARE(changed.count(), 1);
    SitePermissions again;
    view.setPermissions(&again);
    QCOMPARE(view.permissions(), static_cast<QObject *>(&again));
}

// Notifications has own model (WebNotifications asks, notifications page lists) and is
// also site permission: decision in either reaches other; engine told once.
void tst_sitepermissions::notificationsAreKeptInStep()
{
    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    SitePermissions *sites = core.sitePermissions();
    NotificationPermissions *notifications = core.notificationPermissions();
    QSignalSpy toEngine(sites, &SitePermissions::engineRequest);
    QSignalSpy toEngineFromNotifications(notifications, &NotificationPermissions::engineRequest);

    sites->set(SitePermissions::Notifications, Chat, SitePermissions::Block);
    QVERIFY(notifications->isBlocked(Chat));
    QCOMPARE(toEngine.count(), 1);
    QCOMPARE(toEngineFromNotifications.count(), 0);
    sites->set(SitePermissions::Notifications, Chat, SitePermissions::Allow);
    QVERIFY(notifications->isAllowed(Chat));
    QCOMPARE(notifications->allowedCount(), 1);
    sites->set(SitePermissions::Notifications, Chat, SitePermissions::Default);
    QVERIFY(!notifications->isAllowed(Chat));
    QVERIFY(!notifications->isBlocked(Chat));
    QCOMPARE(notifications->rowCount(), 0);
    QCOMPARE(toEngineFromNotifications.count(), 0);

    notifications->setAllowed(News, true);
    QCOMPARE(sites->decision(SitePermissions::Notifications, News), int(SitePermissions::Allow));
    QCOMPARE(toEngineFromNotifications.count(), 1);
    notifications->setAllowed(News, false);
    QCOMPARE(sites->decision(SitePermissions::Notifications, News), int(SitePermissions::Block));
    notifications->remove(News);
    QCOMPARE(sites->decision(SitePermissions::Notifications, News), int(SitePermissions::Default));
    QCOMPARE(sites->exceptionSiteCount(), 0);
    QCOMPARE(toEngine.count(), 3);
    QCOMPARE(toEngineFromNotifications.count(), 3);

    sites->set(SitePermissions::Popups, Chat, SitePermissions::Allow);
    QCOMPARE(notifications->rowCount(), 0);

    const QVariantList list{permission(QStringLiteral("desktop-notification"), Maps, 1),
                            permission(QStringLiteral("popup"), Maps, 2)};
    notifications->observe(Topic, list);
    sites->observe(Topic, list);
    QVERIFY(notifications->isAllowed(Maps));
    QCOMPARE(sites->decision(SitePermissions::Popups, Maps), int(SitePermissions::Block));
    QCOMPARE(sites->decision(SitePermissions::Notifications, Maps), int(SitePermissions::Allow));
}

void tst_sitepermissions::defaults()
{
    QTemporaryDir dir;
    Salama::SettingsSections settings(dir.path() + QStringLiteral("/salama.conf"));
    SitePermissionSettings *sites = settings.sitePermissions();
    QSignalSpy popups(sites, &SitePermissionSettings::popupsAllowedChanged);
    QSignalSpy location(sites, &SitePermissionSettings::locationBlockedChanged);
    QSignalSpy camera(sites, &SitePermissionSettings::cameraBlockedChanged);
    QSignalSpy microphone(sites, &SitePermissionSettings::microphoneBlockedChanged);
    QSignalSpy cookies(sites, &SitePermissionSettings::cookiesChanged);

    QVERIFY(!sites->popupsAllowed());
    QVERIFY(!sites->locationBlocked());
    QVERIFY(!sites->cameraBlocked());
    QVERIFY(!sites->microphoneBlocked());
    QCOMPARE(sites->cookies(), int(SitePermissionSettings::CookiesBlockCrossSite));

    sites->setPopupsAllowed(true);
    sites->setPopupsAllowed(true);
    QCOMPARE(popups.count(), 1);
    sites->setLocationBlocked(true);
    sites->setLocationBlocked(true);
    QCOMPARE(location.count(), 1);
    sites->setCameraBlocked(true);
    QCOMPARE(camera.count(), 1);
    sites->setMicrophoneBlocked(true);
    QCOMPARE(microphone.count(), 1);
    QVERIFY(sites->popupsAllowed() && sites->locationBlocked() && sites->cameraBlocked() &&
            sites->microphoneBlocked());
    sites->setCameraBlocked(false);
    QVERIFY(!sites->cameraBlocked());
    QVERIFY(sites->microphoneBlocked());
    QCOMPARE(camera.count(), 2);

    QCOMPARE(int(SitePermissionSettings::CookiesAllowAll), 0);
    QCOMPARE(int(SitePermissionSettings::CookiesBlockCrossSite), 1);
    QCOMPARE(int(SitePermissionSettings::CookiesBlockAll), 2);
    sites->setCookies(SitePermissionSettings::CookiesBlockAll);
    QCOMPARE(sites->cookies(), 2);
    sites->setCookies(SitePermissionSettings::CookiesBlockAll);
    QCOMPARE(cookies.count(), 1);
    sites->setCookies(3);
    sites->setCookies(-1);
    QCOMPARE(sites->cookies(), 2);
    QCOMPARE(cookies.count(), 1);
    sites->setCookies(SitePermissionSettings::CookiesAllowAll);
    QCOMPARE(sites->cookies(), 0);
    QCOMPARE(cookies.count(), 2);
}

void tst_sitepermissions::defaultsPersist()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/salama.conf");
    {
        Salama::SettingsSections settings(path);
        settings.sitePermissions()->setPopupsAllowed(true);
        settings.sitePermissions()->setLocationBlocked(true);
        settings.sitePermissions()->setMicrophoneBlocked(true);
        settings.sitePermissions()->setCookies(SitePermissionSettings::CookiesAllowAll);
    }
    {
        Salama::SettingsSections again(path);
        QVERIFY(again.sitePermissions()->popupsAllowed());
        QVERIFY(again.sitePermissions()->locationBlocked());
        QVERIFY(!again.sitePermissions()->cameraBlocked());
        QVERIFY(again.sitePermissions()->microphoneBlocked());
        QCOMPARE(again.sitePermissions()->cookies(), int(SitePermissionSettings::CookiesAllowAll));
    }
    {
        QSettings raw(path, QSettings::IniFormat);
        raw.setValue(QStringLiteral("cookies"), 9);
    }
    Salama::SettingsSections again(path);
    QCOMPARE(again.sitePermissions()->cookies(),
             int(SitePermissionSettings::CookiesBlockCrossSite));
}

// Ask each time, any default: engine prompt record, only for kinds pages ask for.
void tst_sitepermissions::askingEachTime()
{
    QVERIFY(SitePermissions::canAsk(SitePermissions::Notifications));
    QVERIFY(SitePermissions::canAsk(SitePermissions::Location));
    QVERIFY(SitePermissions::canAsk(SitePermissions::Camera));
    QVERIFY(SitePermissions::canAsk(SitePermissions::Microphone));
    QVERIFY(!SitePermissions::canAsk(SitePermissions::Popups));
    QVERIFY(!SitePermissions::canAsk(SitePermissions::Cookies));
    QVERIFY(!SitePermissions::canAsk(SitePermissions::TrackingProtection));

    SitePermissions permissions;
    QCOMPARE(permissions.roleNames().value(roleId(SitePermissions::Role::Decision)),
             QByteArray("decision"));
    permissions.observe(Topic, QVariantList{
                                   permission(QStringLiteral("camera"), Chat, 3),
                                   permission(QStringLiteral("desktop-notification"), News, 3),
                                   permission(QStringLiteral("popup"), Maps, 3),
                                   permission(QStringLiteral("cookie"), Maps, 3),
                                   permission(QStringLiteral("trackingprotection"), Maps, 3),
                               });
    QCOMPARE(permissions.rowCount(), 2);
    QCOMPARE(permissions.decision(SitePermissions::Camera, Chat), int(SitePermissions::Ask));
    QCOMPARE(permissions.decision(SitePermissions::Notifications, News), int(SitePermissions::Ask));
    QCOMPARE(permissions.decision(SitePermissions::Popups, Maps), int(SitePermissions::Default));
    const QModelIndex first = permissions.index(0);
    QCOMPARE(permissions.data(first, roleId(SitePermissions::Role::Decision)).toInt(),
             int(SitePermissions::Ask));
    QVERIFY(!permissions.data(first, roleId(SitePermissions::Role::Allowed)).toBool());

    QSignalSpy toEngine(&permissions, &SitePermissions::engineRequest);
    permissions.set(SitePermissions::Location, Maps, SitePermissions::Ask);
    QCOMPARE(permissions.decision(SitePermissions::Location, Maps), int(SitePermissions::Ask));
    QCOMPARE(toEngine.count(), 2);
    for (int i = 0; i < 2; ++i) {
        QCOMPARE(sent(toEngine, i).value(QStringLiteral("msg")).toString(), QStringLiteral("add"));
        QCOMPARE(sent(toEngine, i).value(QStringLiteral("permission")).toInt(), 3);
    }
    permissions.set(SitePermissions::Camera, Chat, SitePermissions::Allow);
    permissions.set(SitePermissions::Camera, Chat, SitePermissions::Ask);
    QCOMPARE(permissions.decision(SitePermissions::Camera, Chat), int(SitePermissions::Ask));
    QCOMPARE(permissions.rowCount(), 3);
    toEngine.clear();
    permissions.set(SitePermissions::Popups, Maps, SitePermissions::Ask);
    QCOMPARE(permissions.decision(SitePermissions::Popups, Maps), int(SitePermissions::Default));
    QCOMPARE(sent(toEngine, 0).value(QStringLiteral("msg")).toString(), QStringLiteral("remove"));

    permissions.set(SitePermissions::Camera, News, SitePermissions::Block);
    permissions.set(SitePermissions::Camera, Maps, SitePermissions::Allow);
    SiteExceptions camera;
    camera.setKind(SitePermissions::Camera);
    camera.setPermissions(&permissions);
    camera.sort(0);
    QCOMPARE(camera.rowCount(), 3);
    QCOMPARE(originAt(camera, 0), Maps);
    QCOMPARE(originAt(camera, 1), News);
    QCOMPARE(originAt(camera, 2), Chat);

    QTemporaryDir dir;
    Core core(dir.path(), dir.path() + QStringLiteral("/salama.conf"), dir.path());
    NotificationPermissions *notifications = core.notificationPermissions();
    notifications->observe(
        Topic, QVariantList{permission(QStringLiteral("desktop-notification"), News, 3),
                            permission(QStringLiteral("desktop-notification"), Chat, 1)});
    QCOMPARE(notifications->rowCount(), 1);
    QVERIFY(!notifications->isBlocked(News));
    core.sitePermissions()->set(SitePermissions::Notifications, Chat, SitePermissions::Ask);
    QVERIFY(!notifications->isAllowed(Chat));
    QVERIFY(!notifications->isBlocked(Chat));
    QCOMPARE(notifications->rowCount(), 0);
}

QTEST_GUILESS_MAIN(tst_sitepermissions)
#include "tst_sitepermissions.moc"
