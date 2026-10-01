// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QSortFilterProxyModel>

namespace Salama {

// The exceptions of one kind of permission, as the page that lists them reads them: the
// sites allowed first, the blocked after them and those asked each time last, each by
// host, as the notifications'
// page lists its own (docs/DECISIONS/0039-site-permissions.md). A view of SitePermissions,
// made in QML for each page:
//
//     SiteExceptions { kind: SitePermissions.Popups; permissions: SitePermissions }
//
// and a site decided on from the page moves under the other heading as its row changes,
// rather than being taken out and put back, so the list keeps the row a finger is on.
class SiteExceptions : public QSortFilterProxyModel
{
    Q_OBJECT
    // A SitePermissions::Kind.
    Q_PROPERTY(int kind READ kind WRITE setKind NOTIFY kindChanged)
    // The model the exceptions are read from. A QObject, as QML on Qt 5.6 hands a model
    // over; anything but a SitePermissions is refused.
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
    // Says so when the number of rows is not what it was.
    void recount();

    int m_kind = 0;
    int m_count = 0;
};

} // namespace Salama
