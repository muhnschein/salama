// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include "Tab.h"

#include <QAbstractListModel>
#include <QList>

namespace Salama {

class TabModel;
class TabPersistence;

// The tabs closed most recently, newest first, for opening again from the panel
// under the grid's foot. Kept to a few dozen and written to the database with the
// tabs, so what was closed before a restart is still there after it.
class ClosedTabModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum class Role
    {
        ClosedId = Qt::UserRole + 1,
        Url,
        Title,
        Favicon
    };

    static const int Limit = 30;

    // A null persistence keeps the list in memory only.
    ClosedTabModel(TabModel *tabs, TabPersistence *persistence);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    const QList<ClosedTab> &closedTabs() const;

    // Opens the tab at this row again, in the current group, and forgets it here.
    Q_INVOKABLE void reopen(int row);
    Q_INVOKABLE void clear();

    // Called by TabModel as a tab closes.
    void record(const Tab &tab);

signals:
    void countChanged();

private:
    TabModel *m_tabs;
    TabPersistence *m_persistence;
    QList<ClosedTab> m_closed;
    int m_nextId = 1;
};

} // namespace Salama
