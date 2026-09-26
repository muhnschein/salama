// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QVariant>

class QSettings;

namespace Salama {

// One settings page's worth of the preferences, kept with the others in the one file in
// the Sailjail-approved config location (docs/DECISIONS/0028-settings-pages.md). A
// section borrows the file; whoever owns the sections owns it, and outlives them.
class SettingsSection : public QObject
{
protected:
    explicit SettingsSection(QSettings &file, QObject *parent = nullptr);

    QVariant value(const char *key, const QVariant &initially = QVariant()) const;
    void setValue(const char *key, const QVariant &value);
    void remove(const char *key);

    // A switch as stored, what it is before it is first switched given. Setting one
    // answers whether that changed it, so the caller knows to say so.
    bool flag(const char *key, bool initially = true) const;
    bool setFlag(const char *key, bool on, bool initially = true);

    // One of the choices from first to last, as stored. Anything else -- the file is
    // one a user can edit -- reads back as what it is before it is first chosen, and
    // is refused. Setting one answers whether that changed it.
    int choice(const char *key, int initially, int first, int last) const;
    bool setChoice(const char *key, int chosen, int initially, int first, int last);

private:
    QSettings &m_file;
};

} // namespace Salama
