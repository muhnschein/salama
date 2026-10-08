// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "settings/SettingsSection.h"

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

namespace Salama {

QString defaultSearchEngine();
QString withoutWww(const QString &host);

// Built-in + site-offered engines. Selection lives in SearchSettings.
class SearchEngines : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(QStringList engineNames READ engineNames NOTIFY enginesChanged)
    Q_PROPERTY(QStringList engineKeys READ engineKeys NOTIFY enginesChanged)
    Q_PROPERTY(QStringList engineHosts READ engineHosts NOTIFY enginesChanged)
    Q_PROPERTY(int addedCount READ addedCount NOTIFY enginesChanged)
    Q_PROPERTY(QVariantList foundEngines READ foundEngines NOTIFY foundChanged)

public:
    explicit SearchEngines(QSettings &file, QObject *parent = nullptr);

    QStringList engineNames() const;
    QStringList engineKeys() const;
    QStringList engineHosts() const;
    int addedCount() const;
    QVariantList foundEngines() const;

    struct Results
    {
        QString host;
        // Or path prefix before words when words in path.
        QString path;
        // Empty when words in path.
        QString parameter;
        QString pathSuffix;
    };

    int indexOf(const QString &key) const;
    int count() const;
    QString keyAt(int index) const;
    QString templateAt(int index) const;

    // href = OpenSearch description url.
    Q_INVOKABLE bool offerEngine(const QString &title, const QString &href, const QString &host);
    // False keeps offer.
    Q_INVOKABLE bool addFoundEngine(const QString &href, const QString &description);
    Q_INVOKABLE void forgetFoundEngine(const QString &href);
    Q_INVOKABLE void removeAddedEngine(const QString &key);
    Q_INVOKABLE void removeAddedEngines();

    // Start page treats searches as not sites.
    bool isSearchUrl(const QString &url) const;

signals:
    void enginesChanged();
    void foundChanged();
    // Emitted before enginesChanged() so selection settles first.
    void engineAdded(const QString &key);

private:
    struct Engine
    {
        QString key;
        QString name;
        QString urlTemplate;
        QString host;
    };

    struct Found
    {
        QString title;
        QString href;
        QString host;
    };

    void rebuildResults();
    QVector<Engine> engines() const;
    bool hasEngineNamed(const QString &name) const;
    QString uniqueKey(const QString &name) const;
    void readStored();
    void store();
    void enginesWereChanged();

    QVector<Engine> m_added;
    QVector<Found> m_found;
    QVector<Results> m_results;
};

} // namespace Salama
