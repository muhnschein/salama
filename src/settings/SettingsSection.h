// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QVariant>

class QSettings;

namespace Salama {

// File borrowed, must outlive section.
class SettingsSection : public QObject
{
protected:
    explicit SettingsSection(QSettings &file, QObject *parent = nullptr);

    QVariant value(const char *key, const QVariant &initially = QVariant()) const;
    void setValue(const char *key, const QVariant &value);
    void remove(const char *key);

    bool flag(const char *key, bool initially = true) const;
    bool setFlag(const char *key, bool on, bool initially = true);

    // Outside [first, last] (user-edited file) reads `initially`; set refused.
    int choice(const char *key, int initially, int first, int last) const;
    bool setChoice(const char *key, int chosen, int initially, int first, int last);

private:
    QSettings &m_file;
};

} // namespace Salama
