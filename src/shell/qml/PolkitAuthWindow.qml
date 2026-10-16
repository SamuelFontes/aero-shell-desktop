/* SPDX-License-Identifier: MIT */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: authWindow
    property var targetScreen
    property var panelWindow
    property bool sessionStarted: false

    screen: targetScreen
    transientParent: panelWindow
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.WindowStaysOnTopHint
    title: "AeroShell Authentication"
    width: 500
    height: sessionStarted ? 330 : 290
    x: targetScreen ? targetScreen.virtualX + Math.round((targetScreen.width - width) / 2) : 100
    y: targetScreen ? targetScreen.virtualY + Math.round((targetScreen.height - height) / 2) : 100
    color: "transparent"
    visible: polkitAgentModel.authenticationActive

    onXChanged: shellBridge.setSurfacePosition(authWindow, Qt.point(x, y))
    onYChanged: shellBridge.setSurfacePosition(authWindow, Qt.point(x, y))

    Component.onCompleted: {
        shellBridge.registerSurface(authWindow, "popup")
        shellBridge.setSurfacePosition(authWindow, Qt.point(x, y))
    }
    onVisibleChanged: {
        if (visible) {
            sessionStarted = false
            passwordField.clear()
            shellBridge.setSurfacePosition(authWindow, Qt.point(x, y))
            requestActivate()
        }
    }

    Connections {
        target: polkitAgentModel
        function onAuthenticationActiveChanged() {
            if (!polkitAgentModel.authenticationActive) {
                authWindow.sessionStarted = false
                passwordField.clear()
            }
        }
        function onPromptChanged() {
            if (polkitAgentModel.waitingForResponse) {
                passwordField.clear()
                passwordField.forceActiveFocus()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 10
        border.color: "#d8ebff"
        border.width: 1
        gradient: Gradient {
            GradientStop { position: 0; color: "#ffffff" }
            GradientStop { position: 0.16; color: "#e0dfde" }
            GradientStop { position: 1; color: "#2c5483" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: "Authentication required"
            color: "#141312"
            font.pixelSize: 20
            font.bold: true
        }

        Text {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            text: polkitAgentModel.message
            textFormat: Text.PlainText
            color: "#141312"
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 3
        }

        RowLayout {
            Layout.fillWidth: true
            visible: !authWindow.sessionStarted
            Text {
                text: "Account"
                color: "#141312"
            }
            ComboBox {
                id: identityPicker
                Layout.fillWidth: true
                model: polkitAgentModel.identities
                currentIndex: 0
                visible: model.length > 1
            }
            Text {
                Layout.fillWidth: true
                visible: identityPicker.model.length === 1
                text: identityPicker.model.length ? identityPicker.model[0] : ""
                color: "#141312"
            }
        }

        Text {
            Layout.fillWidth: true
            visible: authWindow.sessionStarted
            text: polkitAgentModel.prompt || "Enter your password to continue."
            textFormat: Text.PlainText
            color: "#141312"
            wrapMode: Text.WordWrap
        }

        TextField {
            id: passwordField
            Layout.fillWidth: true
            visible: authWindow.sessionStarted && polkitAgentModel.waitingForResponse
            echoMode: polkitAgentModel.promptEcho ? TextInput.Normal : TextInput.Password
            selectByMouse: true
            inputMethodHints: Qt.ImhSensitiveData
            onAccepted: if (visible) authWindow.submitResponse()
        }

        Text {
            Layout.fillWidth: true
            visible: polkitAgentModel.error.length > 0
            text: polkitAgentModel.error
            textFormat: Text.PlainText
            color: "#9d2424"
            wrapMode: Text.WordWrap
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                text: "Cancel"
                onClicked: polkitAgentModel.cancelRequest()
            }
            Button {
                visible: !authWindow.sessionStarted
                text: "Continue"
                onClicked: {
                    if (polkitAgentModel.startAuthentication(identityPicker.currentIndex))
                        authWindow.sessionStarted = true
                }
            }
            Button {
                visible: authWindow.sessionStarted
                enabled: polkitAgentModel.waitingForResponse
                text: "Authenticate"
                onClicked: authWindow.submitResponse()
            }
        }
    }

    function submitResponse() {
        const response = passwordField.text
        passwordField.clear()
        polkitAgentModel.submitResponse(response)
    }
}
