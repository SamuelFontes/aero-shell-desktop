/* SPDX-License-Identifier: MIT */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
Window {
    id: security
    property var targetScreen
    screen: targetScreen
    title: "Windows Security"
    width: targetScreen ? targetScreen.width : 1280
    height: targetScreen ? targetScreen.height : 720
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "#194d7f"
    ColumnLayout {
        anchors.centerIn: parent
        width: 260
        spacing: 15
        Label { text: "Windows Security"; color: "white"; font.pixelSize: 24; Layout.bottomMargin: 20 }
        Button { text: "Lock this computer"; Layout.fillWidth: true; onClicked: { security.hide(); screenLockController.requestLock() } }
        Button { text: "Log off"; Layout.fillWidth: true; onClicked: shellBridge.requestLogout() }
        Button { text: "Start Task Manager"; Layout.fillWidth: true; onClicked: { security.hide(); applicationModel.run("taskmgr") } }
        Button { text: "Cancel"; Layout.fillWidth: true; onClicked: security.hide() }
    }
    Shortcut { sequence: "Escape"; onActivated: security.hide() }
}
