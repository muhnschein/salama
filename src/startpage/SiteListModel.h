// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "ModelRoles.h"

#include <QAbstractListModel>
#include <QList>
#include <QString>

namespace Salama {

struct Site
{
    QString url;
    QString title;
    QString favicon;

    friend bool operator==(const Site &one, const Site &other)
    {
        return one.url == other.url && one.title == other.title && one.favicon == other.favicon;
    }

    friend bool operator!=(const Site &one, const Site &other)
    {
        return !(one == other);
    }
};

class SiteListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum class Role
    {
        Url = Qt::UserRole + 1,
        Title,
        Favicon
    };

    explicit SiteListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    const QList<Site> &sites() const;
    // No reset if unchanged: refreshed on every change.
    void setSites(const QList<Site> &sites);

signals:
    void countChanged();

private:
    QList<Site> m_sites;
};

} // namespace Salama
