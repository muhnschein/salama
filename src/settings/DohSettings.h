// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QStringList>
#include <QVariantList>

namespace Salama {

class DohSettings : public SettingsSection
{
    Q_OBJECT
    Q_PROPERTY(int protection READ protection WRITE setProtection NOTIFY protectionChanged)
    Q_PROPERTY(QString provider READ provider WRITE setProvider NOTIFY providerChanged)
    Q_PROPERTY(bool customProvider READ customProvider NOTIFY providerChanged)
    Q_PROPERTY(QVariantList providers READ providers CONSTANT)
    Q_PROPERTY(QStringList exceptions READ exceptions NOTIFY exceptionsChanged)

public:
    // No Default: needs Mozilla rollout heuristics engine lacks. Stored: file format.
    enum Protection // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        ProtectionOff = 0,
        ProtectionIncreased = 1,
        ProtectionMax = 2
    };
    Q_ENUM(Protection)

    enum ProviderProblem // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        ProviderValid = 0,
        ProviderNotHttps = 1,
        ProviderInvalid = 2
    };
    Q_ENUM(ProviderProblem)

    explicit DohSettings(QSettings &file, QObject *parent = nullptr);

    int protection() const;
    void setProtection(int level);

    QString provider() const;
    void setProvider(const QString &url);
    bool customProvider() const;
    QVariantList providers() const;

    QStringList exceptions() const;

    static QString defaultProvider();

    // Needs "https://" + host (Firefox Android DohUrlValidator).
    Q_INVOKABLE static int providerProblem(const QString &url);

    Q_INVOKABLE static QString domainOf(const QString &text);

    Q_INVOKABLE bool addException(const QString &text);
    Q_INVOKABLE void removeException(const QString &domain);
    Q_INVOKABLE void removeAllExceptions();

signals:
    void protectionChanged();
    void providerChanged();
    void exceptionsChanged();

private:
    void setExceptions(const QStringList &domains);
};

} // namespace Salama
