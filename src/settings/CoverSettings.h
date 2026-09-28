// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QString>
#include <QStringList>

namespace Salama {

// Settings > Cover: the cover's one quick action. What the cover shows is not a setting
// (docs/DECISIONS/0037-cover-is-where-you-were.md); the "coverStyle" key an earlier
// version kept is left in the file, unread.
class CoverSettings : public SettingsSection
{
    Q_OBJECT
    // The cover's one quick action, a QuickAction value, and for QuickActionBookmark the
    // bookmark it opens and the picture it wears (docs/DECISIONS/0029-quick-action.md).
    // The bookmark is kept as its id, and its address and title beside it: the id for
    // as long as the bookmark lives, the address to find it again when it is removed and
    // added back under a new id, the title to name it by once it is gone for good.
    Q_PROPERTY(int quickAction READ quickAction WRITE setQuickAction NOTIFY quickActionChanged)
    Q_PROPERTY(int quickActionBookmark READ quickActionBookmark NOTIFY quickActionBookmarkChanged)
    Q_PROPERTY(QString quickActionBookmarkUrl READ quickActionBookmarkUrl NOTIFY
                   quickActionBookmarkChanged)
    Q_PROPERTY(QString quickActionBookmarkTitle READ quickActionBookmarkTitle NOTIFY
                   quickActionBookmarkChanged)
    Q_PROPERTY(QString quickActionIcon READ quickActionIcon WRITE setQuickActionIcon NOTIFY
                   quickActionIconChanged)
    Q_PROPERTY(QStringList quickActionIcons READ quickActionIcons CONSTANT)

public:
    // What the cover's quick action does: nothing, open the address bar for a new tab,
    // show the bookmarks, open one bookmark, show the downloads, or show the history.
    // Stored, so the numbers are part of the file format. Unscoped on purpose, as
    // ReaderSettings::Colors is (cpp:S3642): QML reaches these as
    // `CoverSettings.QuickActionSearch`, which Qt 5.6 cannot do for a scoped enum.
    enum QuickAction // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        QuickActionNone = 0,
        QuickActionSearch = 1,
        QuickActionBookmarks = 2,
        QuickActionBookmark = 3,
        QuickActionDownloads = 4,
        QuickActionHistory = 5
    };
    Q_ENUM(QuickAction)

    explicit CoverSettings(QSettings &file, QObject *parent = nullptr);

    // Search unless changed: what the cover offered before there was a choice. Out of
    // range reads back as the default: this comes from a file a user can edit.
    int quickAction() const;
    void setQuickAction(int action);
    // 0, with no address and no title, when no bookmark has been picked.
    int quickActionBookmark() const;
    QString quickActionBookmarkUrl() const;
    QString quickActionBookmarkTitle() const;
    // The three together, as a bookmark is picked or found again under a new id: one
    // write, one signal. An id of 0 forgets the bookmark; a negative one is refused.
    // The action itself is left as it is.
    Q_INVOKABLE void setQuickActionBookmark(int id, const QString &url, const QString &title);
    // One of quickActionIcons(), the first unless changed; a name not on the list is
    // refused, and reads back as the first.
    QString quickActionIcon() const;
    void setQuickActionIcon(const QString &name);
    QStringList quickActionIcons() const;
    // The file a cover action's picture is drawn from, relative to the application's
    // root: "art/cover/<name>-<size>-<white|black>.png", at the rendered size nearest the
    // icon size asked for, white for a dark ambience and black for a light one
    // (icons/render.sh draws them).
    Q_INVOKABLE static QString iconPath(const QString &name, qreal iconSize, bool onDark);

signals:
    void quickActionChanged();
    void quickActionBookmarkChanged();
    void quickActionIconChanged();
};

} // namespace Salama
