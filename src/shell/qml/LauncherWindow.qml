/* SPDX-License-Identifier: MIT */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: launcher
    property var targetScreen
    property var panelWindow: null
    property var settingsWindow
    property string lockError: ""

    screen: targetScreen
    transientParent: panelWindow
    readonly property real desiredX: targetScreen ? targetScreen.virtualX + 10 : 10
    readonly property real desiredY: targetScreen && panelWindow ? panelWindow.desiredY - height - 6 : 166
    x: desiredX
    y: desiredY
    width: 462
    height: Math.min(560, targetScreen ? targetScreen.height - 80 : 560)
    visible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    title: "AeroShell Start"

    onDesiredXChanged: shellBridge.setSurfacePosition(launcher, Qt.point(desiredX, desiredY))
    onDesiredYChanged: shellBridge.setSurfacePosition(launcher, Qt.point(desiredX, desiredY))

    Component.onCompleted: {
        shellBridge.registerSurface(launcher, "popup")
        shellBridge.setSurfacePosition(launcher, Qt.point(desiredX, desiredY))
    }
    onActiveChanged: if (visible && !active) hide()
    onVisibleChanged: {
        if (visible) {
            shellBridge.setSurfacePosition(launcher, Qt.point(desiredX, desiredY))
            search.forceActiveFocus()
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 8
        border.color: "#eaf4ff"
        border.width: 1
        gradient: Gradient {
            GradientStop { position: 0; color: "#ffffff" }
            GradientStop { position: 0.12; color: "#d8e5f2" }
            GradientStop { position: 1; color: "#477eb6" }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 5
            color: "#ffffff"
            border.color: "#9bbde0"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8



                ListView {
                    id: applicationList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: applicationModel
                    spacing: 2
                    delegate: ItemDelegate {
                        required property string name
                        required property string storageId
                        required property string comment
                        required property string iconName
                        width: applicationList.width
                        height: 42
                        highlighted: ListView.isCurrentItem
                        onClicked: {
                            applicationModel.launch(storageId)
                            launcher.visible = false
                        }
                        contentItem: RowLayout {
                            spacing: 10
                            Image {
                                id: appIcon
                                width: 34
                                height: 34
                                source: iconName.length ? "image://theme/" + iconName + "?v=" + shellSettings.iconThemeRevision : ""
                                sourceSize: Qt.size(width, height)
                                fillMode: Image.PreserveAspectFit
                                visible: status === Image.Ready
                            }
                            Rectangle {
                                width: 34
                                height: 34
                                radius: 6
                                visible: !appIcon.visible
                                border.color: "#d8ebff"
                                gradient: Gradient {
                                    GradientStop { position: 0; color: "#7fb2e2" }
                                    GradientStop { position: 1; color: "#2b74c7" }
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: name.length ? name.substring(0, 1).toUpperCase() : "•"
                                    color: "white"
                                    font.bold: true
                                }
                            }
                            ColumnLayout {
                                spacing: 1
                                Layout.fillWidth: true
                                Text {
                                    text: name
                                    color: "#141312"
                                    font.pixelSize: 13
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: comment
                            color: "#596b7d"
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar { }
                }
                TextField {
                    id: search
                    Layout.fillWidth: true
                    placeholderText: "Search programs"
                    selectByMouse: true
                    onTextChanged: { applicationModel.query = text; applicationList.currentIndex = 0 }
                    Keys.onDownPressed: { applicationList.currentIndex = Math.min(applicationList.count - 1, applicationList.currentIndex + 1) }
                    Keys.onUpPressed: { applicationList.currentIndex = Math.max(0, applicationList.currentIndex - 1) }
                    onAccepted: {
                        if (applicationList.currentItem) {
                            applicationModel.launch(applicationList.currentItem.storageId)
                            launcher.visible = false
                        }
                    }
                    Keys.onEscapePressed: launcher.visible = false
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 144
            Layout.fillHeight: true
            radius: 5
            gradient: Gradient {
                GradientStop { position: 0; color: "#345f8d" }
                GradientStop { position: 1; color: "#183553" }
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6
                Text {
                    text: "AeroShell"
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 13
                    Layout.alignment: Qt.AlignHCenter
                }
                Repeater {
                    model: [
                        { label: "Home", location: "home", icon: "user-home" },
                        { label: "Documents", location: "documents", icon: "folder-documents" },
                        { label: "Pictures", location: "pictures", icon: "folder-pictures" },
                        { label: "Music", location: "music", icon: "folder-music" },
                        { label: "Downloads", location: "downloads", icon: "folder-downloads" },
                        { label: "Computer", location: "computer", icon: "computer" }
                    ]
                    delegate: Button {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: visible ? 34 : 0
                        visible: modelData.location === "home" || modelData.location === "computer"
                                 || applicationModel.locationAvailable(modelData.location)
                        text: modelData.label
                        icon.name: modelData.icon
                        palette.button: "#2f5e8e"
                        palette.buttonText: "#ffffff"
                        palette.highlight: "#72a9df"
                        palette.highlightedText: "#ffffff"
                        background: Rectangle {
                            radius: 3
                            color: parent.down ? "#1b426e" : parent.hovered ? "#4e7dad" : "transparent"
                        }
                        onClicked: {
                            applicationModel.openLocation(modelData.location)
                            launcher.visible = false
                        }
                    }
                }
                Item { Layout.fillHeight: true }
                Button {
                    Layout.fillWidth: true
                    text: "Settings"
                    palette.button: "#2f5e8e"
                    palette.buttonText: "#ffffff"
                    palette.highlight: "#72a9df"
                    palette.highlightedText: "#ffffff"
                    background: Rectangle {
                        radius: 3
                        color: parent.down ? "#1b426e" : parent.hovered ? "#4e7dad" : "transparent"
                    }
                    onClicked: {
                        launcher.visible = false
                        if (settingsWindow) {
                            settingsWindow.show()
                            settingsWindow.requestActivate()
                        }
                    }
                }
                Button {
                    Layout.fillWidth: true
                    text: "Power…"
                    palette.button: "#2f5e8e"
                    palette.buttonText: "#ffffff"
                    palette.highlight: "#72a9df"
                    palette.highlightedText: "#ffffff"
                    background: Rectangle {
                        radius: 3
                        color: parent.down ? "#1b426e" : parent.hovered ? "#4e7dad" : "transparent"
                    }
                    onClicked: {
                        powerActions.clearError()
                        powerDialog.open()
                    }
                }
                Button {
                    Layout.fillWidth: true
                    text: "Lock"
                    palette.button: "#2f5e8e"
                    palette.buttonText: "#ffffff"
                    palette.highlight: "#72a9df"
                    palette.highlightedText: "#ffffff"
                    background: Rectangle {
                        radius: 3
                        color: parent.down ? "#1b426e" : parent.hovered ? "#4e7dad" : "transparent"
                    }
                    onClicked: {
                        if (screenLockController.requestLock()) {
                            launcher.visible = false
                            launcher.lockError = ""
                        } else {
                            launcher.lockError = screenLockController.error
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: launcher.lockError.length > 0
                    text: launcher.lockError
                    textFormat: Text.PlainText
                    color: "#ffe5e5"
                    wrapMode: Text.WordWrap
                }
                Button {
                    Layout.fillWidth: true
                    text: "Log out"
                    palette.button: "#2f5e8e"
                    palette.buttonText: "#ffffff"
                    palette.highlight: "#72a9df"
                    palette.highlightedText: "#ffffff"
                    background: Rectangle {
                        radius: 3
                        color: parent.down ? "#1b426e" : parent.hovered ? "#4e7dad" : "transparent"
                    }
                    onClicked: shellBridge.requestLogout()
                }
            }
        }
    }

    property string selectedPowerAction: ""

    Dialog {
        id: powerDialog
        implicitWidth: 340
        width: Math.min(implicitWidth, launcher.width - 24)
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: "Power options"
        standardButtons: Dialog.Close

        contentItem: ColumnLayout {
            spacing: 8
            Text {
                Layout.fillWidth: true
                text: powerActions.serviceAvailable
                      ? "Choose a power action for this computer."
                      : "The system power service is unavailable."
                color: "#141312"
                wrapMode: Text.WordWrap
            }
            Button {
                Layout.fillWidth: true
                text: "Suspend"
                enabled: powerActions.suspendAvailable && !powerActions.pending
                onClicked: launcher.confirmPowerAction("suspend")
            }
            Button {
                Layout.fillWidth: true
                text: "Restart"
                enabled: powerActions.rebootAvailable && !powerActions.pending
                onClicked: launcher.confirmPowerAction("reboot")
            }
            Button {
                Layout.fillWidth: true
                text: "Shut down"
                enabled: powerActions.powerOffAvailable && !powerActions.pending
                onClicked: launcher.confirmPowerAction("poweroff")
            }
            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                running: powerActions.pending
                visible: running
            }
            Text {
                Layout.fillWidth: true
                visible: powerActions.error.length > 0
                text: powerActions.error
                textFormat: Text.PlainText
                color: "#9d2424"
                wrapMode: Text.WordWrap
            }
        }
    }

    Dialog {
        id: powerConfirmation
        implicitWidth: 340
        width: Math.min(implicitWidth, launcher.width - 24)
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: "Confirm power action"
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: {
            powerActions.request(launcher.selectedPowerAction)
            powerDialog.open()
        }
        onRejected: powerDialog.open()

        contentItem: Text {
            text: launcher.selectedPowerAction === "suspend" ? "Suspend this computer now?"
                  : launcher.selectedPowerAction === "reboot" ? "Restart this computer now?"
                  : "Shut down this computer now?"
            color: "#141312"
            wrapMode: Text.WordWrap
        }
    }

    function confirmPowerAction(action) {
        selectedPowerAction = action
        powerDialog.close()
        powerConfirmation.open()
    }
}
