// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "TabHistory.h"

#include "EngineData.h"
#include "reader/Reader.h"
#include "storage/Storage.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QVariantMap>
#include <QtDebug>
#include <cmath>
#include <limits>

namespace Salama {

namespace {

const QString AskMessage = QStringLiteral("salama:tabhistory");
const QString ResultMessage = QStringLiteral("salama:tabhistory-result");
const QString GoMessage = QStringLiteral("salama:tabhistory-go");

// Child-side nsISHistory exists while session history stays in content process (embedlite runs
// without Fission, so no sessionHistoryInParent). Moved to parent -> legacySHistory throws ->
// empty answer -> hold acts as tap. Go checks entry still same: list may be stale.
const char *const ScriptTemplate = R"((function () {
    "use strict";
    var ask = "%1", result = "%2", go = "%3", limit = %4;
    function navigation() {
        return docShell.QueryInterface(Components.interfaces.nsIWebNavigation);
    }
    function history() {
        return navigation().sessionHistory.legacySHistory;
    }
    addMessageListener(ask, function () {
        var entries = [];
        try {
            var list = history();
            for (var i = list.index - 1; i >= 0 && entries.length < limit; --i) {
                var entry = list.getEntryAtIndex(i);
                entries.push({ index: i, address: entry.URI.spec, title: entry.title });
            }
        } catch (e) {}
        sendAsyncMessage(result, { entries: entries });
    });
    addMessageListener(go, function (message) {
        try {
            var list = history(), index = message.data.index;
            if (index >= 0 && index < list.index
                    && list.getEntryAtIndex(index).URI.spec === message.data.address) {
                navigation().gotoIndex(index);
            }
        } catch (e) {}
    });
})();
)";

} // namespace

TabHistory::TabHistory(const Storage &storage, QObject *parent)
    : QObject(parent)
    , m_db(storage.database())
{
}

QString TabHistory::script() const
{
    return QString::fromUtf8(ScriptTemplate).arg(AskMessage, ResultMessage, GoMessage).arg(Limit);
}

QString TabHistory::scriptUrl() const
{
    return QStringLiteral("data:application/javascript;charset=utf-8,") +
           QString::fromLatin1(QUrl::toPercentEncoding(script()));
}

QString TabHistory::askMessage() const
{
    return AskMessage;
}

QString TabHistory::resultMessage() const
{
    return ResultMessage;
}

QString TabHistory::goMessage() const
{
    return GoMessage;
}

// Index whole, strictly falling, in int range: anything else not what script sends.
QVariantList TabHistory::entries(const QVariant &data) const
{
    QVariantList entries;
    double previous = double(std::numeric_limits<int>::max()) + 1;
    const QVariantList listed = data.toMap().value(QStringLiteral("entries")).toList();
    for (const QVariant &value : listed) {
        if (entries.count() == Limit) {
            break;
        }
        const QVariantMap entry = value.toMap();
        const QVariant index = entry.value(QStringLiteral("index"));
        const double number = index.toDouble();
        const QString address = entry.value(QStringLiteral("address")).toString();
        const QUrl url(address, QUrl::TolerantMode);
        if (!EngineData::isNumber(index) || number < 0 || number >= previous ||
            number != std::floor(number) || !url.isValid() || url.scheme().isEmpty()) {
            continue;
        }
        previous = number;
        const QString source = Reader::sourceUrl(url);
        const QString shown = source.isEmpty() ? address : source;
        entries.append(QVariantMap{
            {QStringLiteral("index"), int(number)},
            {QStringLiteral("address"), address},
            {QStringLiteral("url"), shown},
            {QStringLiteral("title"), entry.value(QStringLiteral("title")).toString().simplified()},
            {QStringLiteral("favicon"), faviconOf(shown)},
        });
    }
    return entries;
}

// Page's own icon, else site's latest: page reached by script (pushState) never loads, so never
// gets icon of its own.
QString TabHistory::faviconOf(const QString &url) const
{
    const QUrl page(url, QUrl::TolerantMode);
    QString site = page.toString(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment |
                                 QUrl::RemoveUserInfo);
    if (page.host().isEmpty()) {
        site.clear();
    }
    site.replace(QLatin1Char('\\'), QStringLiteral("\\\\"))
        .replace(QLatin1Char('%'), QStringLiteral("\\%"))
        .replace(QLatin1Char('_'), QStringLiteral("\\_"));
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT favicon FROM browser_history WHERE favicon != '' AND "
                                 "(url = ? OR url LIKE ? ESCAPE '\\') "
                                 "ORDER BY url = ? DESC, date DESC LIMIT 1"));
    query.addBindValue(url);
    // NULL when no host: matches nothing.
    query.addBindValue(site.isEmpty() ? QVariant() : QVariant(site + QStringLiteral("/%")));
    query.addBindValue(url);
    if (!query.exec()) {
        qWarning() << "TabHistory:" << query.lastError().text();
        return {};
    }
    return query.next() ? query.value(0).toString() : QString();
}

} // namespace Salama
