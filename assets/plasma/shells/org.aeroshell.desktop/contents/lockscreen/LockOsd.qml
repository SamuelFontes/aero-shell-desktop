/* SPDX-License-Identifier: GPL-2.0-or-later */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Rectangle {
    id: osd
    objectName: "onScreenDisplay"
    property alias timeout: hideTimer.interval
    property var osdValue: 0
    property real osdMaxValue: 100
    property string icon: ""
    property bool showingProgress: true

    visible: false
    width: 320
    height: 84
    radius: 8
    border.color: "#d8ebff"
    gradient: Gradient {
        GradientStop { position: 0; color: "#e4f0fc" }
        GradientStop { position: 1; color: "#7ba0c9" }
    }

    function show() {
        visible = true
        hideTimer.restart()
    }

    Timer {
        id: hideTimer
        interval: 2000
        onTriggered: osd.visible = false
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14
        Kirigami.Icon {
            source: osd.icon
            visible: osd.icon.length > 0
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
        }
        ColumnLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: osd.showingProgress ? Math.round(Number(osd.osdValue)) + "%" : String(osd.osdValue)
                textFormat: Text.PlainText
                color: "#141312"
                elide: Text.ElideRight
            }
            ProgressBar {
                Layout.fillWidth: true
                visible: osd.showingProgress
                from: 0
                to: Math.max(1, osd.osdMaxValue)
                value: osd.showingProgress ? Number(osd.osdValue) : 0
            }
        }
    }
}
