// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include "SettingsSection.h"

#include <QStringList>
#include <QVariantList>

namespace Salama {

// DNS over HTTPS, as Firefox for Android's settings page for it has it: how much it is
// used, which provider answers, and the sites it is not used for
// (docs/DECISIONS/0047-secure-connections.md). What each stands for in the engine's
// preferences is EngineMessages::dohPreferences().
class DohSettings : public SettingsSection
{
    Q_OBJECT
    // A Protection value.
    Q_PROPERTY(int protection READ protection WRITE setProtection NOTIFY protectionChanged)
    // The address of the provider that answers, one of the built-in providers' or the
    // reader's own: Cloudflare's until another is chosen, as in Firefox.
    Q_PROPERTY(QString provider READ provider WRITE setProvider NOTIFY providerChanged)
    // Whether the provider is one the reader gave, not one of the built-in ones.
    Q_PROPERTY(bool customProvider READ customProvider NOTIFY providerChanged)
    // Firefox for Android's built-in providers, the default first, as {name, url}.
    Q_PROPERTY(QVariantList providers READ providers CONSTANT)
    // The domains DNS over HTTPS is not used for, with their subdomains, in the order
    // they were added.
    Q_PROPERTY(QStringList exceptions READ exceptions NOTIFY exceptionsChanged)

public:
    // Firefox's protection levels without Default, which is Firefox's own judgment of when
    // to turn DNS over HTTPS on, made by Mozilla's rollout and heuristics that the engine
    // here does not run. Stored, so the numbers are part of the file format, and unscoped
    // as PrivacySettings::TrackingProtection is.
    enum Protection // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        ProtectionOff = 0,
        ProtectionIncreased = 1,
        ProtectionMax = 2
    };
    Q_ENUM(Protection)

    // What is wrong with an address given for a provider, as Firefox for Android tells it.
    enum ProviderProblem // NOSONAR(cpp:S3642) QML on Qt 5.6 reads no scoped enum
    {
        ProviderValid = 0,
        ProviderNotHttps = 1,
        ProviderInvalid = 2
    };
    Q_ENUM(ProviderProblem)

    explicit DohSettings(QSettings &file, QObject *parent = nullptr);

    // Off unless changed. Out of range reads back as the default.
    int protection() const;
    void setProtection(int level);

    // A provider that is not a valid address is refused.
    QString provider() const;
    void setProvider(const QString &url);
    bool customProvider() const;
    QVariantList providers() const;

    QStringList exceptions() const;

    // Cloudflare's, Firefox's default provider.
    static QString defaultProvider();

    // What is wrong with an address for a provider: it has to start with "https://" and
    // name a host (Firefox for Android's DohUrlValidator).
    Q_INVOKABLE static int providerProblem(const QString &url);

    // The domain in what was typed for an exception, lower case, a scheme or a path
    // around it dropped: "https://Example.com/a" is "example.com". Empty when there is
    // none.
    Q_INVOKABLE static QString domainOf(const QString &text);

    // Adds the domain in what was typed; one already there or none at all is not added.
    // Answers whether it was.
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
