pragma Singleton
import QtQuick 2.6

QtObject {
    property var notifications: []
    // Subscribed topics, in order, repeats kept.
    property var observers: []

    // Engine message on subscribed topic; tests raise it.
    signal recvObserve(string message, var data)

    function addObserver(topic) {
        var list = observers
        list.push(topic)
        observers = list
    }

    function notifyObservers(topic, value) {
        var list = notifications
        list.push({ "topic": topic, "value": value })
        notifications = list
    }
}
