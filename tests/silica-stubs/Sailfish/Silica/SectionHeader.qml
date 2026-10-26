import QtQuick 2.6

// As Silica: Label inset page margin each side, text right-aligned.
Item {
    property string text

    x: Theme.horizontalPageMargin
    width: (parent ? parent.width : 0) - 2 * x
}
