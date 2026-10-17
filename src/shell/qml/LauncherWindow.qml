/* SPDX-License-Identifier: MIT */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import org.kde.ksvg as KSvg

Window {
    id: launcher
    property var targetScreen
    property var panelWindow: null
    property var settingsWindow
    property string lockError: ""

    screen: targetScreen
    transientParent: panelWindow
    readonly property real desiredX: targetScreen ? targetScreen.virtualX : 0
    readonly property real desiredY: targetScreen && panelWindow ? panelWindow.desiredY - height : 166
    x: desiredX
    y: desiredY
    width: 414
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

    KSvg.FrameSvgItem {
        anchors.fill: parent
        imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-dialog.svgz"
    }
    // This overlay is the original SevenStart glass shading.
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: 8
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#26000000" }
            GradientStop { position: 0.11; color: "#53000000" }
            GradientStop { position: 0.2775; color: "#82000000" }
            GradientStop { position: 0.445; color: "#99000000" }
            GradientStop { position: 0.51; color: "#99000000" }
            GradientStop { position: 0.84; color: "#4f000000" }
            GradientStop { position: 1.0; color: "#00000000" }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 5

        KSvg.FrameSvgItem {
            Layout.preferredWidth: 254
            Layout.fillHeight: true
            imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-background.svg"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4



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
                        height: 38
                        highlighted: ListView.isCurrentItem
                        onClicked: {
                            applicationModel.launch(storageId)
                            launcher.visible = false
                        }
                        background: KSvg.FrameSvgItem {
                            imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-menuitem.svg"
                            prefix: "hover"
                            visible: parent.hovered || parent.highlighted
                        }
                        contentItem: RowLayout {
                            spacing: 8
                            Image {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                source: "image://theme/" + (iconName.length ? iconName : "application-x-generic") + "?v=" + shellSettings.iconThemeRevision
                                sourceSize: Qt.size(32, 32)
                                fillMode: Image.PreserveAspectFit
                            }
                            Text {
                                Layout.fillWidth: true
                                text: name
                                color: "#141312"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
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
                    Layout.preferredHeight: 25
                    background: KSvg.FrameSvgItem {
                        imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-lineedit.svg"
                        prefix: "base"
                    }
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

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 3
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

                        contentItem: Text {
                        text: parent.text
                        leftPadding: 8
                        color: "white"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        verticalAlignment: Text.AlignVCenter
                    }
                    palette.button: "#2f5e8e"
                        palette.buttonText: "#ffffff"
                        palette.highlight: "#72a9df"
                        palette.highlightedText: "#ffffff"
                        background: KSvg.FrameSvgItem {
                            imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-sidebaritem.svg"
                            prefix: "menuitem"
                            visible: parent.hovered || parent.down || parent.activeFocus
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
                    contentItem: Text {
                        text: parent.text
                        leftPadding: 8
                        color: "white"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        verticalAlignment: Text.AlignVCenter
                    }
                    palette.button: "#2f5e8e"
                    palette.buttonText: "#ffffff"
                    palette.highlight: "#72a9df"
                    palette.highlightedText: "#ffffff"
                    background: KSvg.FrameSvgItem {
                            imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-sidebaritem.svg"
                            prefix: "menuitem"
                            visible: parent.hovered || parent.down || parent.activeFocus
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
                    contentItem: Text {
                        text: parent.text
                        leftPadding: 8
                        color: "white"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        verticalAlignment: Text.AlignVCenter
                    }
                    palette.button: "#2f5e8e"
                    palette.buttonText: "#ffffff"
                    palette.highlight: "#72a9df"
                    palette.highlightedText: "#ffffff"
                    background: KSvg.FrameSvgItem {
                        imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-startmenu-buttons.svg"
                        prefix: parent.down ? "pressed" : parent.hovered ? "hover" : "normal"
                    }
                    onClicked: {
                        powerActions.clearError()
                        powerDialog.open()
                    }
                }
                Button {
                    Layout.fillWidth: true
                    text: "Lock"
                    contentItem: Text {
                        text: parent.text
                        leftPadding: 8
                        color: "white"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        verticalAlignment: Text.AlignVCenter
                    }
                    palette.button: "#2f5e8e"
                    palette.buttonText: "#ffffff"
                    palette.highlight: "#72a9df"
                    palette.highlightedText: "#ffffff"
                    background: KSvg.FrameSvgItem {
                            imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-sidebaritem.svg"
                            prefix: "menuitem"
                            visible: parent.hovered || parent.down || parent.activeFocus
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
                    contentItem: Text {
                        text: parent.text
                        leftPadding: 8
                        color: "white"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        verticalAlignment: Text.AlignVCenter
                    }
                    palette.button: "#2f5e8e"
                    palette.buttonText: "#ffffff"
                    palette.highlight: "#72a9df"
                    palette.highlightedText: "#ffffff"
                    background: KSvg.FrameSvgItem {
                            imagePath: aeroThemeAssetsPath + "/widgets/aerothemeplasma/start-sidebaritem.svg"
                            prefix: "menuitem"
                            visible: parent.hovered || parent.down || parent.activeFocus
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
