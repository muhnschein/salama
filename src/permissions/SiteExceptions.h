// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QSortFilterProxyModel>

namespace Salama {

// Changed decision moves row (not remove+insert): keeps row under finger.
class SiteExceptions : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int kind READ kind WRITE setKind NOTIFY kindChanged)
    // QObject: Qt 5.6 QML passes models so. Non-SitePermissions refused.
    Q_PROPERTY(QObject *permissions READ permissions WRITE setPermissions NOTIFY permissionsChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit SiteExceptions(QObject *parent = nullptr);

    int kind() const;
    void setKind(int kind);
    QObject *permissions() const;
    void setPermissions(QObject *permissions);

signals:
    void kindChanged();
    void permissionsChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    void recount();

    int m_kind = 0;
    int m_count = 0;
};

} // namespace Salama
