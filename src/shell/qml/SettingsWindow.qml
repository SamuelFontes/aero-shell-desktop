/* SPDX-License-Identifier: MIT */
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: settingsWindow
    property var targetScreen
    readonly property string screenName: targetScreen ? targetScreen.name : "default"

    title: "AeroShell Settings"
    width: 500
    height: 610
    minimumWidth: 440
    minimumHeight: 420
    screen: targetScreen
    visible: false

    Rectangle {
        anchors.fill: parent
        color: "#e0dfde"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        Text {
            text: "AeroShell Settings"
            color: "#141312"
            font.pixelSize: 20
            font.bold: true
        }

        ScrollView {
            id: settingsScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: settingsScroll.availableWidth
                spacing: 12

                GroupBox {
                    title: "Desktop background"
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        Text {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: "#596a7c"
                            text: "AeroThemePlasma is the default appearance. The selected picture is saved only in the AeroShell profile; reset to restore the bundled wallpaper."
                        }

                        RowLayout {
                            Button {
                                text: "Choose picture…"
                                onClicked: wallpaperDialog.open()
                            }
                            Button {
                                text: "Reset to Aero default"
                                onClicked: shellSettings.resetWallpaper(settingsWindow.screenName)
                            }
                        }
                    }
                }

                GroupBox {
                    title: "Taskbar widgets"
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Text {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: "#596a7c"
                            text: "All supported widgets start enabled. These choices affect AeroShell panels only."
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 2
                            columnSpacing: 16

                            CheckBox {
                                text: "Network"
                                checked: shellSettings.panelWidgets.network
                                onToggled: shellSettings.setPanelWidgetVisible("network", checked)
                            }
                            CheckBox {
                                text: "Volume"
                                checked: shellSettings.panelWidgets.audio
                                onToggled: shellSettings.setPanelWidgetVisible("audio", checked)
                            }
                            CheckBox {
                                text: "Battery"
                                checked: shellSettings.panelWidgets.battery
                                onToggled: shellSettings.setPanelWidgetVisible("battery", checked)
                            }
                            CheckBox {
                                text: "System tray"
                                checked: shellSettings.panelWidgets.tray
                                onToggled: shellSettings.setPanelWidgetVisible("tray", checked)
                            }
                            CheckBox {
                                text: "Keyboard layout"
                                checked: shellSettings.panelWidgets.keyboard
                                onToggled: shellSettings.setPanelWidgetVisible("keyboard", checked)
                            }
                            CheckBox {
                                text: "Notifications"
                                checked: shellSettings.panelWidgets.notifications
                                onToggled: shellSettings.setPanelWidgetVisible("notifications", checked)
                            }
                            CheckBox {
                                text: "Clock and calendar"
                                checked: shellSettings.panelWidgets.clock
                                onToggled: shellSettings.setPanelWidgetVisible("clock", checked)
                            }
                            Button {
                                text: "Restore default widgets"
                                onClicked: shellSettings.resetPanelWidgets()
                            }
                        }
                    }
                }

                GroupBox {
                    title: "Authentication"
                    Layout.fillWidth: true

                    Text {
                        anchors.fill: parent
                        anchors.margins: 8
                        text: polkitAgentModel.status
                        color: polkitAgentModel.registered ? "#385d31" : "#9d2424"
                        wrapMode: Text.WordWrap
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button {
                text: "Close"
                onClicked: settingsWindow.close()
            }
        }
    }

    FileDialog {
        id: wallpaperDialog
        title: "Choose an AeroShell desktop picture"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp)"]
        onAccepted: shellSettings.setWallpaper(settingsWindow.screenName, selectedFile)
    }
}
