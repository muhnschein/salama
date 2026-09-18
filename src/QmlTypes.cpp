// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "QmlTypes.h"

#include "Core.h"
#include "tabs/ClosedTabModel.h"
#include "tabs/GroupTabModel.h"
#include "tabs/TabGroupModel.h"

#include <QQmlEngine>
#include <qqml.h>

namespace Salama {

namespace {

const char *const ModuleUri = "harbour.salama";

Core *coreInstance = nullptr;

QObject *keepOwnership(QObject *object)
{
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    return object;
}

QObject *tabModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->tabs());
}

QObject *groupTabModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->tabs()->groupTabs());
}

QObject *closedTabModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->tabs()->closedTabs());
}

QObject *tabGroupModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->tabs()->groupModel());
}

QObject *tabSearchModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->tabSearch());
}

QObject *historyModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->history());
}

QObject *bookmarkModelProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->bookmarks());
}

QObject *settingsProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->settings());
}

QObject *engineMessagesProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return keepOwnership(coreInstance->engineMessages());
}

} // namespace

void registerQmlTypes(Core *core)
{
    static bool registered = false;
    coreInstance = core;
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
    qmlRegisterSingletonType<Settings>(ModuleUri, 1, 0, "Settings", &settingsProvider);
    qmlRegisterSingletonType<EngineMessages>(ModuleUri, 1, 0, "EngineMessages",
                                             &engineMessagesProvider);
}

} // namespace Salama
