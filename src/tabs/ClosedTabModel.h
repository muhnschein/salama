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

// Recently closed tabs, newest first, capped, persisted.
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

    // Null persistence = memory only.
    ClosedTabModel(TabModel *tabs, TabPersistence *persistence);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    const QList<ClosedTab> &closedTabs() const;

    Q_INVOKABLE void reopen(int row);
    Q_INVOKABLE void clear();

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
