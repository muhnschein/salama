// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QAbstractListModel>
#include <QList>

namespace Salama {

class TabModel;

// Current group's tabs. Id list, not filter proxy: proxy turns move into layout change,
// rebuilding every cell incl. one under finger.
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
    // Swaps in TabModel too so order persists.
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
