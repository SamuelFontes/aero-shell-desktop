/* SPDX-License-Identifier: MIT */
import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    id: desktop
    property var targetScreen
    property url defaultWallpaperUrl
    property url selectedWallpaperUrl

    screen: targetScreen
    readonly property real desiredX: targetScreen ? targetScreen.virtualX : 0
    readonly property real desiredY: targetScreen ? targetScreen.virtualY : 0
    x: desiredX
    y: desiredY
    width: targetScreen ? targetScreen.width : 1280
    height: targetScreen ? targetScreen.height : 720
    visible: false
    color: "#22456d"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus
    title: "AeroShell Desktop"

    onDesiredXChanged: shellBridge.setSurfacePosition(desktop, Qt.point(desiredX, desiredY))
    onDesiredYChanged: shellBridge.setSurfacePosition(desktop, Qt.point(desiredX, desiredY))

    Component.onCompleted: {
        shellBridge.registerSurface(desktop, "desktop")
        shellBridge.setSurfacePosition(desktop, Qt.point(desiredX, desiredY))
        refreshWallpaper()
    }

    function refreshWallpaper() {
        selectedWallpaperUrl = shellSettings.wallpaperUrl(targetScreen ? targetScreen.name : "default")
    }

    Connections {
        target: shellSettings
        function onSettingsChanged() { desktop.refreshWallpaper() }
    }

    Rectangle {
        anchors.fill: parent
        color: "#22456d"
        Image {
            anchors.fill: parent
            source: desktop.selectedWallpaperUrl.toString().length > 0
                    ? desktop.selectedWallpaperUrl : desktop.defaultWallpaperUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            visible: status === Image.Ready
        }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: desktopMenu.popup()
    }
    Menu {
        id: desktopMenu
        popupType: Popup.Window
        MenuItem { text: "Refresh"; onTriggered: desktopItemModel.refresh() }
        MenuItem { text: "New folder"; onTriggered: desktopItemModel.newFolder() }
        MenuSeparator {}
        MenuItem { text: "Open Desktop in Explorer"; onTriggered: applicationModel.openLocation("desktop") }
        MenuItem { text: "Personalize"; onTriggered: shellActions.dispatch("settings") }
    }
    GridView {
        id: desktopItems
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.topMargin: 14
        anchors.leftMargin: 8
        anchors.bottomMargin: 58
        width: Math.min(parent.width, Math.ceil(count / Math.max(1, Math.floor(height / cellHeight))) * cellWidth)
        cellWidth: 92
        cellHeight: 92
        flow: GridView.TopToBottom
        model: desktopItemModel
        interactive: false

        delegate: Item {
            required property string display
            required property url url
            required property string iconName
            property bool selected: false
            width: desktopItems.cellWidth
            height: desktopItems.cellHeight

            Rectangle {
                anchors.fill: parent
                anchors.margins: 2
                radius: 3
                color: selected ? "#40689499" : "transparent"
                border.color: selected ? "#d8ebff" : "transparent"
            }

            Image {
                id: desktopItemIcon
                anchors.top: parent.top
                anchors.topMargin: 4
                anchors.horizontalCenter: parent.horizontalCenter
                width: 42
                height: 42
                source: iconName.length ? "image://theme/" + iconName + "?v=" + shellSettings.iconThemeRevision : ""
                sourceSize: Qt.size(width, height)
                fillMode: Image.PreserveAspectFit
                visible: status === Image.Ready
            }

            Text {
                anchors.centerIn: desktopItemIcon
                visible: !desktopItemIcon.visible
                text: display.length ? display.substring(0, 1).toUpperCase() : "•"
                color: "white"
                font.bold: true
                font.pixelSize: 21
            }

            Text {
                anchors.top: desktopItemIcon.bottom
                anchors.topMargin: 3
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 3
                anchors.rightMargin: 3
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                text: display
                color: "white"
                style: Text.Outline
                styleColor: "#23364c"
                font.pixelSize: 11
            }

            MouseArea {
                anchors.fill: parent
                onClicked: parent.selected = true
                onDoubleClicked: desktopItemModel.open(url)
        }
        }
    }

    }
}
