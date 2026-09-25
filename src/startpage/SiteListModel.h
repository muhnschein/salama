// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

namespace Salama {

// A page the start page offers: where it is, what it is called and its icon, any of
// the last two possibly empty.
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

// One of the start page's lists, as StartPage reads it (src/startpage/StartPage.h).
class SiteListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    // Unscoped, as every other model's roles are, and not the oversight SonarQube
    // reads it as (cpp:S3642): a role is an int wherever Qt handles one -- data()'s
    // argument, roleNames()' keys, Qt::UserRole it starts from -- and a scoped enum
    // would need a cast at each of them.
    enum Role
    {
        UrlRole = Qt::UserRole + 1,
        TitleRole,
        FaviconRole
    };

    explicit SiteListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    const QList<Site> &sites() const;
    // Resets the model, unless the list is the one it already holds: the start page
    // is read again on every visit, and a list that has not changed is not built again.
    void setSites(const QList<Site> &sites);

signals:
    void countChanged();

private:
    QList<Site> m_sites;
};

} // namespace Salama
