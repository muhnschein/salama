// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace Salama {

// A link shared to the browser from another application's share sheet: the page it
// points at opens in a new tab (docs/DECISIONS/0042-share-target.md).
//
// The share sheet offers this browser for links alone -- harbour-salama.desktop's one
// share method takes text/x-url and nothing else -- and calls it over D-Bus: the method
// share(a{sv}) of the interface org.sailfishos.share, on the object /share/<method>, at
// the service named for the application's Sailjail organisation and name. The platform's
// own receiver for that call, Sailfish.Share's ShareProvider, takes a resource only as a
// file or as a name and its data, and a link shared from a browser -- sailfish-browser's
// or this one's own ShareAction -- is neither: {type, status, linkTitle}, the address
// being the status. So the call is answered here, as harbour-nextmarks answers it
// (src/sharereceiver.cpp, which captured the shape on a device).
class ShareReceiver : public QObject
{
    Q_OBJECT

public:
    explicit ShareReceiver(QObject *parent = nullptr);

    // Claims the object and then the service on the session bus, so that a share that
    // started the application finds it answering. False where there is no session bus
    // to claim them on -- a host's tests -- or another process holds the name.
    bool registerService();

    // The window is made and listening: a link shared before it was is handed on now.
    Q_INVOKABLE void setReady();

    // What one share call carries. A link worth opening is handed on, or kept until the
    // window is ready; anything else is dropped.
    void receive(const QVariantMap &arguments);

    // The address a share call carries: its first resource's status, as a browser
    // shares a link, or its data, as a ShareProvider-shaped sender would, trimmed --
    // and only if it is an http or https address with a host. Anything else is empty:
    // the browser opens links, and nothing that merely came as text.
    static QString sharedUrl(const QVariantMap &arguments);

    // Qt's D-Bus reading of a{sv} leaves a nested container as a QDBusArgument rather
    // than a list or a map; this reads it all the way down.
    static QVariant unwrap(const QVariant &value);

signals:
    void linkShared(const QString &url);

private:
    bool m_ready = false;
    QString m_pending;
};

} // namespace Salama
