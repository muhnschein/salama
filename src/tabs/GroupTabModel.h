// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QAbstractListModel>
#include <QList>

namespace Salama {

class TabModel;

// The tabs of the current group, in the order the tab model keeps them: what the grid
// shows. A list of ids over the tab model rather than a filter proxy, because a proxy
// answers a move in its source with a layout change, and a layout change rebuilds
// every cell of the grid -- including the one a finger is carrying
// (docs/DECISIONS/0015-tab-groups.md). The tab model owns this list and calls the
// methods under "Kept by TabModel" as it changes; nothing else does.
class GroupTabModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit GroupTabModel(TabModel *tabs);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    Q_INVOKABLE int tabIdAt(int row) const;
    Q_INVOKABLE int rowOf(int tabId) const;
    // Reorder within the group, from the grid. The tabs trade places in the tab model
    // too, so the order is what is persisted.
    Q_INVOKABLE void moveTab(int from, int to);

    // Kept by TabModel.
    void reset(const QList<int> &tabIds);
    void append(int tabId);
    void remove(int tabId);
    void moveTabRow(int from, int to);
    void changed(int tabId, int role);

signals:
    void countChanged();

private:
    TabModel *m_tabs;
    QList<int> m_tabIds;
};

} // namespace Salama
