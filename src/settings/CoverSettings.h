// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QString>
#include <QStringList>

namespace Salama {

// Settings > Cover: what the cover shows, and its one quick action.
class CoverSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(int style READ style WRITE setStyle NOTIFY styleChanged)
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
    // What the cover shows; see docs/DECISIONS/0031-cover-is-lightning.md. The values
    // are stored, so their numbers are part of the file format. 0 and 1 are the numbers
    // the icon-only and last-tab covers had, so a reader who chose the one gets the
    // lightning and a reader who chose the other keeps it; the every-tab cover's 2 is
    // out of range now, and reads back as the lightning as any such value does.
    //
    // Unscoped on purpose, as ReaderSettings::Colors is (cpp:S3642): the cover reaches
    // these as `CoverSettings.LatestTab`, which Qt 5.6 cannot do for a scoped enum.
    enum Style // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        Lightning = 0,
        LatestTab = 1
    };
    Q_ENUM(Style)

    // What the cover's quick action does: nothing, open the address bar for a new tab,
    // show the bookmarks, open one bookmark, show the downloads, or show the history.
    // Stored, and unscoped as Style is, for the same reason.
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

    // Out-of-range values read back as the default rather than as a cover that draws
    // nothing: this comes from a file a user can edit.
    int style() const;
    void setStyle(int style);

    // Search unless changed: what the cover offered before there was a choice. Out of
    // range reads back as the default, like style.
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
    void styleChanged();
    void quickActionChanged();
    void quickActionBookmarkChanged();
    void quickActionIconChanged();
};

} // namespace Salama
