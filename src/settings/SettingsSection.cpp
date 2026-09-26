// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "SettingsSection.h"

#include <QSettings>

namespace Salama {

SettingsSection::SettingsSection(QSettings &file, QObject *parent)
    : QObject(parent)
    , m_file(file)
{
}

QVariant SettingsSection::value(const char *key, const QVariant &initially) const
{
    return m_file.value(QLatin1String(key), initially);
}

void SettingsSection::setValue(const char *key, const QVariant &value)
{
    m_file.setValue(QLatin1String(key), value);
}

void SettingsSection::remove(const char *key)
{
    m_file.remove(QLatin1String(key));
}

bool SettingsSection::flag(const char *key, bool initially) const
{
    return value(key, initially).toBool();
}

bool SettingsSection::setFlag(const char *key, bool on, bool initially)
{
    if (on == flag(key, initially)) {
        return false;
    }
    setValue(key, on);
    return true;
}

int SettingsSection::choice(const char *key, int initially, int first, int last) const
{
    const int stored = value(key, initially).toInt();
    return stored < first || stored > last ? initially : stored;
}

bool SettingsSection::setChoice(const char *key, int chosen, int initially, int first, int last)
{
    if (chosen < first || chosen > last || chosen == choice(key, initially, first, last)) {
        return false;
    }
    setValue(key, chosen);
    return true;
}

} // namespace Salama
