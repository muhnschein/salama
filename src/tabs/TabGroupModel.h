// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include <QAbstractListModel>
#include <QString>

namespace Salama {

class TabModel;

// The tab groups, in the order the strip above the grid shows them. The rows are the
// tab model's own list of groups; this is the shape QML reads it in, and the place the
// group actions are reached from (docs/DECISIONS/0015-tab-groups.md).
class TabGroupModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum class Role
    {
        GroupId = Qt::UserRole + 1,
        Name,
        TabCount,
        Current,
        // The group every ordinary tab starts in, which is neither renamed nor removed.
        Default,
        // The previews of the group's most recent tabs, up to PreviewLimit, the most
        // recent first: the picture of the group on the Tab groups page. A tab with no
        // preview is an empty string, and a group with no tabs has none.
        Previews
    };

    // The picture of a group is the preview of the tab last in front in it, as the
    // Gallery shows an album by one of its photos.
    static const int PreviewLimit = 1;

    explicit TabGroupModel(TabModel *tabs);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    Q_INVOKABLE int groupIdAt(int row) const;
    // Makes the group at this row the current one: the grid shows it, and its most
    // recent tab comes to the front.
    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE int addGroup(const QString &name);
    Q_INVOKABLE void renameGroup(int groupId, const QString &name);
    Q_INVOKABLE bool removeGroup(int groupId);
    // Puts the group's tabs in the default group and removes it; the tabs stay open.
    Q_INVOKABLE bool ungroup(int groupId);
    // Reorder, from the Tab groups page. The default group stays first.
    Q_INVOKABLE bool moveGroup(int from, int to);
    Q_INVOKABLE bool moveTab(int tabId, int groupId);

    // Kept by TabModel.
    void inserted(int row);
    void removed(int row);
    void moved(int from, int to);
    void changed(int row, Role role);
    void changedAll(Role role);

signals:
    void countChanged();

private:
    TabModel *m_tabs;
};

} // namespace Salama
