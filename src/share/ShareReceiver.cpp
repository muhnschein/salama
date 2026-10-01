// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#include "ShareReceiver.h"

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QUrl>
#include <QVariantList>

namespace Salama {

namespace {

// harbour-salama.desktop's [X-Sailjail] OrganizationName.ApplicationName, which is the
// name its ExecDBus starts the application under, and its X-Share-Methods' one method.
const char *const ServiceName = "io.github.muhnschein.salama";
const char *const ObjectPath = "/share/link";

// The interface the share sheet calls. A slot exported on a plain object is not
// advertised under an interface name, and the caller's call then fails as
// UnknownInterface: an adaptor is what names it.
class ShareAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfishos.share")

public:
    ShareAdaptor(QObject *target, ShareReceiver *receiver)
        : QDBusAbstractAdaptor(target)
        , m_receiver(receiver)
    {
    }

public slots:
    void share(const QVariantMap &arguments)
    {
        m_receiver->receive(arguments);
    }

private:
    ShareReceiver *m_receiver;
};

} // namespace

ShareReceiver::ShareReceiver(QObject *parent)
    : QObject(parent)
{
}

bool ShareReceiver::registerService()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return false;
    }
    auto *target = new QObject(this);
    new ShareAdaptor(target, this);
    // The object first: claiming the name is what lets a call that started the
    // application through, and it has to find something answering.
    return bus.registerObject(QLatin1String(ObjectPath), target) &&
           bus.registerService(QLatin1String(ServiceName));
}

void ShareReceiver::setReady()
{
    m_ready = true;
    if (!m_pending.isEmpty()) {
        const QString url = m_pending;
        m_pending.clear();
        emit linkShared(url);
    }
}

void ShareReceiver::receive(const QVariantMap &arguments)
{
    const QString url = sharedUrl(arguments);
    if (url.isEmpty()) {
        return;
    }
    // Answered at once whether or not the window is there: the share sheet gives up on a
    // call that a starting application is slow to answer.
    if (m_ready) {
        emit linkShared(url);
    } else {
        m_pending = url;
    }
}

QString ShareReceiver::sharedUrl(const QVariantMap &arguments)
{
    const QVariantList resources = unwrap(arguments.value(QStringLiteral("resources"))).toList();
    if (resources.isEmpty()) {
        return {};
    }
    QVariant first = unwrap(resources.first());
    // Tolerates one more wrapping, a list around the resource, in case a sender adds it.
    if (first.type() == QVariant::List) {
        const QVariantList nested = first.toList();
        first = nested.isEmpty() ? QVariant() : unwrap(nested.first());
    }
    const QVariantMap resource = unwrap(first).toMap();
    QString text = unwrap(resource.value(QStringLiteral("status"))).toString().trimmed();
    if (text.isEmpty()) {
        const QVariant data = unwrap(resource.value(QStringLiteral("data")));
        text = (data.type() == QVariant::ByteArray ? QString::fromUtf8(data.toByteArray())
                                                   : data.toString())
                   .trimmed();
    }
    const QUrl url(text, QUrl::StrictMode);
    const QString scheme = url.scheme().toLower();
    if (!url.isValid() || url.host().isEmpty() ||
        (scheme != QLatin1String("http") && scheme != QLatin1String("https"))) {
        return {};
    }
    return url.toString();
}

QVariant ShareReceiver::unwrap(const QVariant &value)
{
    if (value.userType() != qMetaTypeId<QDBusArgument>()) {
        return value;
    }
    const auto argument = value.value<QDBusArgument>();
    switch (argument.currentType()) {
    case QDBusArgument::ArrayType:
        return qdbus_cast<QVariantList>(argument);
    case QDBusArgument::MapType:
        return qdbus_cast<QVariantMap>(argument);
    case QDBusArgument::VariantType: {
        QVariant inner;
        argument >> inner;
        return inner;
    }
    default:
        return argument.asVariant();
    }
}

} // namespace Salama

#include "ShareReceiver.moc"
