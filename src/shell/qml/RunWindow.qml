/* SPDX-License-Identifier: MIT */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: run
    property var targetScreen
    screen: targetScreen
    title: "Run"
    width: 420
    height: 185
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    color: "#f0f0f0"
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    onVisibleChanged: if (visible) { command.forceActiveFocus(); command.selectAll() }
    onClosing: event => { event.accepted = false; hide() }
    function execute() {
        if (applicationModel.run(command.text)) hide()
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        Label { Layout.fillWidth: true; text: "Type the name of a program or folder to open."; wrapMode: Text.WordWrap }
        RowLayout {
            Label { text: "Open:" }
            TextField { id: command; Layout.fillWidth: true; selectByMouse: true; onAccepted: run.execute() }
        }
        Item { Layout.fillHeight: true }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button { text: "OK"; enabled: command.text.trim().length > 0; onClicked: run.execute() }
            Button { text: "Cancel"; onClicked: run.hide() }
        }
    }
    Shortcut { sequence: "Escape"; onActivated: run.hide() }
}
