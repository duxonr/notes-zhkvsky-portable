import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

ScrollBar {
    id: scrollBarControl
    x: parent.mirrored ? 0 : parent.width - width
    height: parent.availableHeight
    width: 14
    padding: 2
    hoverEnabled: true
    property var themeData: ({theme: "Light"})
    property bool isDarkGray: true
    property bool showBackground: false
    property color trackColor: themeData.theme === "Dark" ? "#202226" : themeData.theme === "Sepia" ? "#eee1c8" : "#eceef0"
    property color thumbColor: themeData.theme === "Dark" ? "#737d89" : themeData.theme === "Sepia" ? "#9b8f7d" : "#8a939e"

    background: Rectangle {
        implicitWidth: 14
        color: scrollBarControl.trackColor
        radius: 7
        visible: scrollBarControl.showBackground && scrollBarControl.size < 1.0
    }
    contentItem: Rectangle {
        implicitWidth: 10
        radius: 5
        color: scrollBarControl.pressed ? "#2f73b2" : scrollBarControl.hovered ? "#448ac9" : scrollBarControl.thumbColor
        opacity: scrollBarControl.size < 1.0 ? 1 : 0
    }
}
