// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include <QAbstractListModel>
#include <QString>

namespace Salama {

class TabModel;

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
        Default,
        // Newest first, max PreviewLimit. No preview = ""
        Previews
    };

    static const int PreviewLimit = 4;

    explicit TabGroupModel(TabModel *tabs);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    Q_INVOKABLE int groupIdAt(int row) const;
    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE int addGroup(const QString &name);
    Q_INVOKABLE void renameGroup(int groupId, const QString &name);
    Q_INVOKABLE bool removeGroup(int groupId);
    Q_INVOKABLE bool ungroup(int groupId);
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
