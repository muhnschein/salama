// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "CoverSettings.h"

#include <algorithm>

namespace Salama {

namespace {

const char *const QuickActionKey = "quickAction";
const char *const QuickActionBookmarkKey = "quickActionBookmark";
const char *const QuickActionBookmarkUrlKey = "quickActionBookmarkUrl";
const char *const QuickActionBookmarkTitleKey = "quickActionBookmarkTitle";
const char *const QuickActionIconKey = "quickActionIcon";

// Star last: it's bookmarks overview glyph; as default would read as overview.
const QStringList &quickActionIconNames()
{
    static const QStringList names{
        QStringLiteral("globe"), QStringLiteral("heart"), QStringLiteral("home"),
        QStringLiteral("work"),  QStringLiteral("news"),  QStringLiteral("music"),
        QStringLiteral("shop"),  QStringLiteral("star"),
    };
    return names;
}

} // namespace

CoverSettings::CoverSettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
}

int CoverSettings::quickAction() const
{
    return choice(QuickActionKey, QuickActionSearch, QuickActionNone, QuickActionHistory);
}

void CoverSettings::setQuickAction(int action)
{
    if (setChoice(QuickActionKey, action, QuickActionSearch, QuickActionNone, QuickActionHistory)) {
        emit quickActionChanged();
    }
}

int CoverSettings::quickActionBookmark() const
{
    return std::max(value(QuickActionBookmarkKey, 0).toInt(), 0);
}

QString CoverSettings::quickActionBookmarkUrl() const
{
    return value(QuickActionBookmarkUrlKey).toString();
}

QString CoverSettings::quickActionBookmarkTitle() const
{
    return value(QuickActionBookmarkTitleKey).toString();
}

void CoverSettings::setQuickActionBookmark(int id, const QString &url, const QString &title)
{
    if (id < 0) {
        return;
    }
    // id 0 clears url+title too.
    const QString keptUrl = id == 0 ? QString() : url;
    const QString keptTitle = id == 0 ? QString() : title;
    if (id == quickActionBookmark() && keptUrl == quickActionBookmarkUrl() &&
        keptTitle == quickActionBookmarkTitle()) {
        return;
    }
    setValue(QuickActionBookmarkKey, id);
    setValue(QuickActionBookmarkUrlKey, keptUrl);
    setValue(QuickActionBookmarkTitleKey, keptTitle);
    emit quickActionBookmarkChanged();
}

QString CoverSettings::quickActionIcon() const
{
    const QString stored = value(QuickActionIconKey).toString();
    return quickActionIconNames().contains(stored) ? stored : quickActionIconNames().first();
}

void CoverSettings::setQuickActionIcon(const QString &name)
{
    if (!quickActionIconNames().contains(name) || name == quickActionIcon()) {
        return;
    }
    setValue(QuickActionIconKey, name);
    emit quickActionIconChanged();
}

QStringList CoverSettings::quickActionIcons() const
{
    return quickActionIconNames();
}

// Home screen draws cover icons unscaled: snap to rendered 32-64 px step 8. Clamp first.
QString CoverSettings::iconPath(const QString &name, qreal iconSize, bool onDark)
{
    const int size = qRound(qBound(qreal(32), iconSize, qreal(64)) / 8) * 8;
    return QStringLiteral("art/cover/%1-%2-%3.png")
        .arg(name)
        .arg(size)
        .arg(onDark ? QStringLiteral("white") : QStringLiteral("black"));
}

} // namespace Salama
