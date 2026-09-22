pragma Singleton
import QtQuick 2.6

QtObject {
    property var notifications: []
    // The topics subscribed to, in order, repeats included.
    property var observers: []

    // What the engine sends on a subscribed topic; a test raises it.
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
