// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "CoverSettings.h"

#include <algorithm>

namespace Salama {

namespace {

const char *const StyleKey = "coverStyle";
const char *const QuickActionKey = "quickAction";
const char *const QuickActionBookmarkKey = "quickActionBookmark";
const char *const QuickActionBookmarkUrlKey = "quickActionBookmarkUrl";
const char *const QuickActionBookmarkTitleKey = "quickActionBookmarkTitle";
const char *const QuickActionIconKey = "quickActionIcon";

// The pictures a bookmark's quick action can wear, drawn in icons/cover/. The star
// last: the bookmarks overview's own glyph is a star, as the menu sheet's Bookmarks is,
// and a bookmark that wore it by default would read as the overview.
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

int CoverSettings::style() const
{
    return choice(StyleKey, Lightning, Lightning, LatestTab);
}

void CoverSettings::setStyle(int style)
{
    if (setChoice(StyleKey, style, Lightning, Lightning, LatestTab)) {
        emit styleChanged();
    }
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
    // No bookmark has no address and no title either: what is forgotten is forgotten
    // whole, and nothing is left to find it again by.
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

// The home screen draws a cover action's picture from its file as it is, unscaled, so
// the picture has to be drawn at the size it is shown at: icons/render.sh draws each
// glyph at every size from 32 to 64 pixels in steps of 8, which is where Silica's small
// icon falls on the phones this is for, and the size asked for is snapped to the
// nearest of those -- halfway rounds up, as Math.round() does in QML -- and kept
// within them. Bounded before rounding, so no size, however wild, has no int to round
// to.
QString CoverSettings::iconPath(const QString &name, qreal iconSize, bool onDark)
{
    const int size = qRound(qBound(qreal(32), iconSize, qreal(64)) / 8) * 8;
    return QStringLiteral("art/cover/%1-%2-%3.png")
        .arg(name)
        .arg(size)
        .arg(onDark ? QStringLiteral("white") : QStringLiteral("black"));
}

} // namespace Salama
