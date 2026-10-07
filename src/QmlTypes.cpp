// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "QmlTypes.h"

#include "Core.h"
#include "permissions/SiteExceptions.h"
#include "tabs/ClosedTabModel.h"
#include "tabs/GroupTabModel.h"
#include "tabs/TabGroupModel.h"

#include <QQmlEngine>
#include <qqml.h>

namespace Salama {

namespace {

const char *const ModuleUri = "harbour.salama";

// Qt 5.6 provider is plain function pointer, no capture: Core kept here.
Core *&registeredCore()
{
    static Core *core = nullptr;
    return core;
}

QObject *keepOwnership(QObject *object)
{
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    return object;
}

QObject *tabModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->tabs());
}

QObject *groupTabModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->tabs()->groupTabs());
}

QObject *closedTabModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->tabs()->closedTabs());
}

QObject *tabGroupModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->tabs()->groupModel());
}

QObject *tabSearchModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->tabSearch());
}

QObject *historyModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->history());
}

QObject *bookmarkModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->bookmarks());
}

QObject *downloadModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->downloads());
}

QObject *settingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->settings());
}

QObject *searchEnginesProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->searchEngines());
}

QObject *searchSettingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->searchSettings());
}

QObject *readerSettingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->readerSettings());
}

QObject *coverSettingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->coverSettings());
}

QObject *privacySettingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->privacySettings());
}

QObject *dohSettingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->dohSettings());
}

QObject *startPageSettingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->startPageSettings());
}

QObject *sitePermissionSettingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->sitePermissionSettings());
}

QObject *sitePermissionsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->sitePermissions());
}

QObject *omnibarProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->omnibar());
}

QObject *engineMessagesProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->engineMessages());
}

QObject *pageActivityProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->pageActivity());
}

QObject *pageMediaProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->pageMedia());
}

QObject *readerProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->reader());
}

QObject *startPageProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->startPage());
}

QObject *notificationPermissionsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->notificationPermissions());
}

QObject *shareReceiverProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->shareReceiver());
}

QObject *webNotificationsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(registeredCore()->webNotifications());
}

} // namespace

void registerQmlTypes(Core *core)
{
    static bool registered = false;
    registeredCore() = core;
    if (registered) {
        return;
    }
    registered = true;
    qmlRegisterSingletonType<TabModel>(ModuleUri, 1, 0, "TabModel", &tabModelProvider);
    qmlRegisterSingletonType<GroupTabModel>(ModuleUri, 1, 0, "GroupTabs", &groupTabModelProvider);
    qmlRegisterSingletonType<TabGroupModel>(ModuleUri, 1, 0, "TabGroups", &tabGroupModelProvider);
    qmlRegisterSingletonType<ClosedTabModel>(ModuleUri, 1, 0, "ClosedTabs",
                                             &closedTabModelProvider);
    qmlRegisterSingletonType<TabSearchModel>(ModuleUri, 1, 0, "TabSearch", &tabSearchModelProvider);
    qmlRegisterSingletonType<HistoryModel>(ModuleUri, 1, 0, "HistoryModel", &historyModelProvider);
    qmlRegisterSingletonType<BookmarkModel>(ModuleUri, 1, 0, "BookmarkModel",
                                            &bookmarkModelProvider);
    qmlRegisterSingletonType<DownloadModel>(ModuleUri, 1, 0, "DownloadModel",
                                            &downloadModelProvider);
    qmlRegisterSingletonType<Settings>(ModuleUri, 1, 0, "Settings", &settingsProvider);
    qmlRegisterSingletonType<SearchEngines>(ModuleUri, 1, 0, "SearchEngines",
                                            &searchEnginesProvider);
    qmlRegisterSingletonType<SearchSettings>(ModuleUri, 1, 0, "SearchSettings",
                                             &searchSettingsProvider);
    qmlRegisterSingletonType<ReaderSettings>(ModuleUri, 1, 0, "ReaderSettings",
                                             &readerSettingsProvider);
    qmlRegisterSingletonType<CoverSettings>(ModuleUri, 1, 0, "CoverSettings",
                                            &coverSettingsProvider);
    qmlRegisterSingletonType<PrivacySettings>(ModuleUri, 1, 0, "PrivacySettings",
                                              &privacySettingsProvider);
    qmlRegisterSingletonType<DohSettings>(ModuleUri, 1, 0, "DohSettings", &dohSettingsProvider);
    qmlRegisterSingletonType<StartPageSettings>(ModuleUri, 1, 0, "StartPageSettings",
                                                &startPageSettingsProvider);
    qmlRegisterSingletonType<SitePermissionSettings>(ModuleUri, 1, 0, "SitePermissionSettings",
                                                     &sitePermissionSettingsProvider);
    qmlRegisterSingletonType<OmnibarModel>(ModuleUri, 1, 0, "Omnibar", &omnibarProvider);
    qmlRegisterSingletonType<EngineMessages>(ModuleUri, 1, 0, "EngineMessages",
                                             &engineMessagesProvider);
    qmlRegisterSingletonType<PageActivity>(ModuleUri, 1, 0, "PageActivity", &pageActivityProvider);
    qmlRegisterSingletonType<PageMedia>(ModuleUri, 1, 0, "PageMedia", &pageMediaProvider);
    qmlRegisterSingletonType<Reader>(ModuleUri, 1, 0, "Reader", &readerProvider);
    qmlRegisterSingletonType<StartPage>(ModuleUri, 1, 0, "StartPage", &startPageProvider);
    qmlRegisterSingletonType<NotificationPermissions>(ModuleUri, 1, 0, "NotificationPermissions",
                                                      &notificationPermissionsProvider);
    qmlRegisterSingletonType<ShareReceiver>(ModuleUri, 1, 0, "ShareReceiver",
                                            &shareReceiverProvider);
    qmlRegisterSingletonType<WebNotifications>(ModuleUri, 1, 0, "WebNotifications",
                                               &webNotificationsProvider);
    qmlRegisterSingletonType<SitePermissions>(ModuleUri, 1, 0, "SitePermissions",
                                              &sitePermissionsProvider);
    qmlRegisterType<SiteExceptions>(ModuleUri, 1, 0, "SiteExceptions");
    qmlRegisterUncreatableType<SiteListModel>(ModuleUri, 1, 0, "SiteListModel",
                                              QStringLiteral("A list of the start page's"));
}

} // namespace Salama
