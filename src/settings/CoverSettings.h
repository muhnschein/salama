// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QString>
#include <QStringList>

namespace Salama {

// Legacy "coverStyle" key left in file, unread.
class CoverSettings : public SettingsSection
{
    Q_OBJECT
    // Bookmark stored as id + url + title: id while alive, url to refind after re-add under
    // new id, title to name it once gone.
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
    // Stored: values are file format.
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

    int quickAction() const;
    void setQuickAction(int action);
    int quickActionBookmark() const;
    QString quickActionBookmarkUrl() const;
    QString quickActionBookmarkTitle() const;
    // One write, one signal. 0 forgets; negative refused.
    Q_INVOKABLE void setQuickActionBookmark(int id, const QString &url, const QString &title);
    QString quickActionIcon() const;
    void setQuickActionIcon(const QString &name);
    QStringList quickActionIcons() const;
    // Nearest size rendered by icons/render.sh.
    Q_INVOKABLE static QString iconPath(const QString &name, qreal iconSize, bool onDark);

signals:
    void quickActionChanged();
    void quickActionBookmarkChanged();
    void quickActionIconChanged();
};

} // namespace Salama
