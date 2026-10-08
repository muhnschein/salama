// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "DohSettings.h"

#include <QUrl>
#include <QVariantMap>

namespace Salama {

namespace {

const char *const ProtectionKey = "dohProtection";
const char *const ProviderKey = "dohProvider";
const char *const ExceptionsKey = "dohExceptions";

const char *const HttpsPrefix = "https://";

// Firefox Android providers: Gecko default (network.trr.default_provider_uri) + NextDNS.
const char *const CloudflareUrl = "https://mozilla.cloudflare-dns.com/dns-query";
const char *const NextDnsUrl = "https://firefox.dns.nextdns.io/";

} // namespace

DohSettings::DohSettings(QSettings &file, QObject *parent)
    : SettingsSection(file, parent)
{
}

int DohSettings::protection() const
{
    return choice(ProtectionKey, ProtectionOff, ProtectionOff, ProtectionMax);
}

void DohSettings::setProtection(int level)
{
    if (setChoice(ProtectionKey, level, ProtectionOff, ProtectionOff, ProtectionMax)) {
        emit protectionChanged();
    }
}

QString DohSettings::defaultProvider()
{
    return QLatin1String(CloudflareUrl);
}

QString DohSettings::provider() const
{
    const QString stored = value(ProviderKey).toString();
    return providerProblem(stored) == ProviderValid ? stored : defaultProvider();
}

void DohSettings::setProvider(const QString &url)
{
    const QString chosen = url.trimmed();
    if (providerProblem(chosen) != ProviderValid || chosen == provider()) {
        return;
    }
    setValue(ProviderKey, chosen);
    emit providerChanged();
}

bool DohSettings::customProvider() const
{
    const QString current = provider();
    return current != QLatin1String(CloudflareUrl) && current != QLatin1String(NextDnsUrl);
}

QVariantList DohSettings::providers() const
{
    return {
        QVariantMap{{QStringLiteral("name"), QStringLiteral("Cloudflare")},
                    {QStringLiteral("url"), QLatin1String(CloudflareUrl)}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("NextDNS")},
                    {QStringLiteral("url"), QLatin1String(NextDnsUrl)}},
    };
}

int DohSettings::providerProblem(const QString &url)
{
    // Prefix check before parse, as Firefox Android: else "https:/host" passes.
    if (!url.startsWith(QLatin1String(HttpsPrefix))) {
        return ProviderNotHttps;
    }
    const QUrl parsed(url.trimmed(), QUrl::StrictMode);
    if (!parsed.isValid() || parsed.scheme() != QLatin1String("https") || parsed.host().isEmpty()) {
        return ProviderInvalid;
    }
    return ProviderValid;
}

QString DohSettings::domainOf(const QString &text)
{
    const QString typed = text.trimmed();
    if (typed.isEmpty()) {
        return {};
    }
    const int scheme = typed.indexOf(QLatin1String("://"));
    const QString rest = scheme < 0 ? typed : typed.mid(scheme + 3);
    const QUrl parsed(QLatin1String(HttpsPrefix) + rest, QUrl::StrictMode);
    // QUrl lowercases host, matching engine.
    return parsed.isValid() ? parsed.host() : QString();
}

QStringList DohSettings::exceptions() const
{
    return value(ExceptionsKey).toStringList();
}

void DohSettings::setExceptions(const QStringList &domains)
{
    if (domains.isEmpty()) {
        remove(ExceptionsKey);
    } else {
        setValue(ExceptionsKey, domains);
    }
    emit exceptionsChanged();
}

bool DohSettings::addException(const QString &text)
{
    const QString domain = domainOf(text);
    QStringList domains = exceptions();
    if (domain.isEmpty() || domains.contains(domain)) {
        return false;
    }
    domains.append(domain);
    setExceptions(domains);
    return true;
}

void DohSettings::removeException(const QString &domain)
{
    QStringList domains = exceptions();
    if (domains.removeAll(domain) > 0) {
        setExceptions(domains);
    }
}

void DohSettings::removeAllExceptions()
{
    if (!exceptions().isEmpty()) {
        setExceptions({});
    }
}

} // namespace Salama
