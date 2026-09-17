// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
#pragma once

#include <QAbstractListModel>
#include <QString>

namespace Tuuli {

class TabModel;

// The tab groups, in the order the strip above the grid shows them. The rows are the
// tab model's own list of groups; this is the shape QML reads it in, and the place the
// group actions are reached from (docs/DECISIONS/0015-tab-groups.md).
class TabGroupModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role
    {
        GroupIdRole = Qt::UserRole + 1,
        NameRole,
        TabCountRole,
        CurrentRole,
        PrivateRole
    };

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
    Q_INVOKABLE bool moveTab(int tabId, int groupId);

    // Kept by TabModel.
    void inserted(int row);
    void removed(int row);
    void changed(int row, int role);
    void changedAll(int role);

signals:
    void countChanged();

private:
    TabModel *m_tabs;
};

} // namespace Tuuli
