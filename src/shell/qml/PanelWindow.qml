/* SPDX-License-Identifier: MIT */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: panel
    property var targetScreen
    property var launcherWindow
    readonly property int panelHeight: 48
    property date displayedMonth: new Date(new Date().getFullYear(), new Date().getMonth(), 1)
    property string pendingNetworkSsid: ""
    property string pendingNetworkSecurity: ""
    property var toastNotification: ({})

    function showTrayMenu(button, serviceId, menuPath) {
        if (!menuPath.length) {
            const point = button.mapToGlobal(Qt.point(button.width / 2, button.height / 2))
            statusNotifierModel.contextMenu(serviceId, Math.round(point.x), Math.round(point.y))
            return
        }
        remoteMenuModel.openMenu(serviceId, menuPath)
        const point = button.mapToItem(panel.contentItem, 0, 0)
        trayMenuPopup.x = Math.max(6, Math.min(panel.width - trayMenuPopup.width - 6,
                                                point.x + button.width / 2 - trayMenuPopup.width / 2))
        trayMenuPopup.open()
    }

    screen: targetScreen
    x: targetScreen ? targetScreen.geometry.x : 0
    y: targetScreen ? targetScreen.geometry.y + targetScreen.geometry.height - panelHeight : 672
    width: targetScreen ? targetScreen.geometry.width : 1280
    height: panelHeight
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    title: "AeroShell Panel"

    Component.onCompleted: {
        shellBridge.registerSurface(panel, "panel")
        shellBridge.setSurfacePosition(panel, Qt.point(x, y))
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#d9e9f9" }
            GradientStop { position: 0.12; color: "#779bc5" }
            GradientStop { position: 0.48; color: "#3e6e9e" }
            GradientStop { position: 0.52; color: "#2e5782" }
            GradientStop { position: 0.92; color: "#20446f" }
            GradientStop { position: 1.0; color: "#122b49" }
        }
        border.color: "#d8ebff"
        border.width: 1
        opacity: 0.97
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 12
        spacing: 8

        Button {
            id: startButton
            text: "Start"
            Layout.preferredWidth: 48
            Layout.fillHeight: true
            onClicked: {
                if (launcherWindow) {
                    launcherWindow.visible = !launcherWindow.visible
                    if (launcherWindow.visible)
                        launcherWindow.requestActivate()
                }
            }
            contentItem: Item {
                Image {
                    id: startOrbIcon
                    anchors.centerIn: parent
                    width: 42
                    height: 42
                    source: aeroThemeAssetsPath + "/widgets/aerothemeplasma-start-orb.png"
                    sourceClipRect: Qt.rect(0, startButton.down ? 108 : startButton.hovered ? 54 : 0, 54, 54)
                    sourceSize: Qt.size(54, 162)
                    fillMode: Image.PreserveAspectFit
                    visible: status === Image.Ready
                }
                Text {
                    anchors.centerIn: parent
                    text: "A"
                    visible: !startOrbIcon.visible
                    color: "white"
                    font.bold: true
                    font.pixelSize: 22
                }
            }
            background: Rectangle {
                radius: 5
                color: startButton.down ? "#315e89" : startButton.hovered ? "#204876" : "transparent"
            }
        }

        Rectangle { Layout.fillHeight: true; width: 1; color: "#85a6cb"; opacity: 0.6 }

        ListView {
            id: tasks
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            spacing: 5
            clip: true
            model: aeroWindowModel
            delegate: Button {
                id: taskButton
                required property string display
                required property string appId
                required property bool isActive
                required property bool isMinimized
                required property bool isMaximized
                required property bool isCloseable
                required property bool skipTaskbar
                required property int index
                visible: !skipTaskbar
                width: visible ? Math.min(190, Math.max(92, implicitWidth)) : 0
                height: 38
                text: display
                onClicked: {
                    if (isActive && !isMinimized)
                        aeroWindowModel.requestToggleMinimized(index)
                    else
                        aeroWindowModel.requestActivate(index)
                }
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: taskMenu.popup()
                }
                Menu {
                    id: taskMenu
                    MenuItem {
                        text: isMinimized ? "Restore" : "Minimize"
                        onTriggered: aeroWindowModel.requestToggleMinimized(index)
                    }
                    MenuItem {
                        text: isMaximized ? "Restore down" : "Maximize"
                        onTriggered: aeroWindowModel.requestToggleMaximized(index)
                    }
                    MenuSeparator { }
                    MenuItem {
                        text: "Close window"
                        enabled: isCloseable
                        onTriggered: aeroWindowModel.requestClose(index)
                    }
                }
                background: Rectangle {
                    radius: 4
                    border.color: isActive ? "#c4e0ff" : "#71869d"
                    border.width: 1
                    gradient: Gradient {
                        GradientStop { position: 0; color: isActive ? "#91bce6" : "#315e89" }
                        GradientStop { position: 1; color: isActive ? "#2b74c7" : "#204876" }
                    }
                }
                contentItem: RowLayout {
                    spacing: 6
                    Image {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        source: appId.length ? `image://theme/${appId.replace(/\.desktop$/, "")}` : ""
                        sourceSize: Qt.size(width, height)
                        fillMode: Image.PreserveAspectFit
                        visible: status === Image.Ready
                    }
                    Text {
                        Layout.fillWidth: true
                        text: taskButton.text
                        color: "#ffffff"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        leftPadding: 4
                        rightPadding: 4
                        font.pixelSize: 12
                    }
                }
            }
        }

        Button {
            text: shellBridge.showingDesktop ? "▢" : "▣"
            Layout.preferredWidth: 38
            Layout.fillHeight: true
            onClicked: shellBridge.toggleShowDesktop()
            ToolTip.visible: hovered
            ToolTip.text: "Show desktop"
        }

        Rectangle { Layout.fillHeight: true; width: 1; color: "#85a6cb"; opacity: 0.6 }

        ToolButton {
            id: networkButton
            visible: shellSettings.panelWidgets.network
            Layout.preferredWidth: visible ? 38 : 0
            Layout.fillHeight: true
            onClicked: networkPopup.open()
            ToolTip.visible: hovered
            ToolTip.text: networkStatus.summary
            background: Rectangle {
                radius: 4
                color: networkButton.down ? "#418bd4" : networkButton.hovered ? "#497caf" : "transparent"
            }
            contentItem: Item {
                Image {
                    id: networkIcon
                    anchors.centerIn: parent
                    width: 21
                    height: 21
                    source: networkStatus.connected ? "image://theme/network-wireless" : "image://theme/network-offline"
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.PreserveAspectFit
                    visible: status === Image.Ready
                }
                Text {
                    anchors.centerIn: parent
                    visible: !networkIcon.visible
                    text: networkStatus.available ? (networkStatus.connected ? "●" : "○") : "×"
                    color: "white"
                    font.pixelSize: 17
                }
            }
        }

        Popup {
            id: networkPopup
            x: panel.width - width - 182
            y: -height - 8
            width: 310
            height: 460
            padding: 12
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            onOpened: if (networkStatus.available && networkStatus.wifiEnabled) networkStatus.scanWifi()

            background: Rectangle {
                radius: 7
                color: "#ffffff"
                border.color: "#9bbde0"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Network"
                        color: "#141312"
                        font.bold: true
                        font.pixelSize: 15
                    }
                    ToolButton {
                        text: "Refresh"
                        enabled: !networkStatus.busy
                        onClicked: networkStatus.refresh()
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: networkStatus.summary
                    color: "#596a7c"
                    elide: Text.ElideRight
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Wi-Fi"
                        color: "#141312"
                    }
                    Switch {
                        checked: networkStatus.wifiEnabled
                        enabled: networkStatus.available && !networkStatus.busy
                        onToggled: networkStatus.setWifiEnabled(checked)
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Available Wi-Fi networks"
                        color: "#68798a"
                        font.pixelSize: 11
                        font.bold: true
                    }
                    ToolButton {
                        text: networkStatus.busy ? "Scanning…" : "Scan"
                        enabled: networkStatus.available && networkStatus.wifiEnabled && !networkStatus.busy
                        onClicked: networkStatus.scanWifi()
                    }
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 124
                    clip: true
                    model: networkStatus.wifiNetworks
                    delegate: ItemDelegate {
                        required property string ssid
                        required property int signal
                        required property string security
                        required property bool inUse
                        width: ListView.view.width
                        height: 38
                        enabled: networkStatus.available && !networkStatus.busy
                        text: `${inUse ? "● " : ""}${ssid}${security.length ? " · 🔒" : ""} · ${signal}%`
                        onClicked: {
                            if (security.length === 0) {
                                networkStatus.joinWifi(ssid, "", security)
                            } else {
                                panel.pendingNetworkSsid = ssid
                                panel.pendingNetworkSecurity = security
                                wifiPasswordDialog.open()
                            }
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: networkStatus.wifiNetworks.length === 0
                        text: networkStatus.wifiEnabled ? "Choose Scan to look for networks" : "Wi-Fi is turned off"
                        color: "#6f8092"
                        font.pixelSize: 11
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: "Saved connections"
                    color: "#68798a"
                    font.pixelSize: 11
                    font.bold: true
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 132
                    clip: true
                    model: networkStatus.connections
                    delegate: ItemDelegate {
                        required property string name
                        required property string uuid
                        required property string type
                        required property bool active
                        width: ListView.view.width
                        height: 42
                        enabled: networkStatus.available && !networkStatus.busy
                        text: active ? `${name} · Connected` : name
                        onClicked: networkStatus.toggleConnection(uuid, active)
                        ToolTip.visible: hovered
                        ToolTip.text: active ? `Disconnect ${name}` : `Connect ${name}`
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: networkStatus.connections.length === 0
                        text: networkStatus.available ? "No saved connections" : "NetworkManager is unavailable"
                        color: "#6f8092"
                        font.pixelSize: 12
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: networkStatus.error.length > 0
                    text: networkStatus.error
                    color: "#a52b2b"
                    wrapMode: Text.Wrap
                    font.pixelSize: 11
                }
            }
        }

        Dialog {
            id: wifiPasswordDialog
            title: `Connect to ${panel.pendingNetworkSsid}`
            modal: true
            standardButtons: Dialog.Ok | Dialog.Cancel
            anchors.centerIn: Overlay.overlay
            onAccepted: {
                networkStatus.joinWifi(panel.pendingNetworkSsid, wifiPasswordField.text, panel.pendingNetworkSecurity)
                wifiPasswordField.clear()
            }
            onRejected: wifiPasswordField.clear()
            TextField {
                id: wifiPasswordField
                width: 270
                placeholderText: "Wi-Fi password"
                echoMode: TextInput.Password
                onAccepted: wifiPasswordDialog.accept()
            }
        }

        ToolButton {
            id: audioButton
            visible: audioControl.available && shellSettings.panelWidgets.audio
            Layout.preferredWidth: visible ? 38 : 0
            Layout.fillHeight: true
            onClicked: audioPopup.open()
            ToolTip.visible: hovered
            ToolTip.text: audioControl.muted ? "Audio muted" : `Volume ${audioControl.volume}%`
            contentItem: Image {
                id: volumeIcon
                anchors.centerIn: parent
                width: 22
                height: 22
                source: audioControl.muted ? "image://theme/audio-volume-muted" : "image://theme/audio-volume-high"
                sourceSize: Qt.size(width, height)
                fillMode: Image.PreserveAspectFit
                visible: status === Image.Ready
            }
            background: Rectangle {
                radius: 4
                color: audioButton.down ? "#418bd4" : audioButton.hovered ? "#497caf" : "transparent"
            }
            Text {
                anchors.centerIn: parent
                visible: !volumeIcon.visible
                text: audioControl.muted ? "×" : "♪"
                color: "white"
                font.pixelSize: 19
            }
        }

        Popup {
            id: audioPopup
            x: panel.width - width - 48
            y: -height - 8
            width: 232
            height: 100
            padding: 12
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            background: Rectangle {
                radius: 7
                color: "#ffffff"
                border.color: "#9bbde0"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Volume"
                        color: "#141312"
                        font.bold: true
                    }
                    Text {
                        text: `${Math.round(volumeSlider.value)}%`
                        color: "#596a7c"
                        font.pixelSize: 11
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    ToolButton {
                        text: audioControl.muted ? "Unmute" : "Mute"
                        onClicked: audioControl.toggleMute()
                    }
                    Slider {
                        id: volumeSlider
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        stepSize: 1
                        value: audioControl.volume
                        onMoved: volumeUpdateTimer.restart()
                    }
                }
            }
        }

        ToolButton {
            id: batteryButton
            visible: batteryStatus.present && shellSettings.panelWidgets.battery
            Layout.preferredWidth: visible ? 58 : 0
            Layout.fillHeight: true
            onClicked: batteryPopup.open()
            ToolTip.visible: hovered
            ToolTip.text: `${batteryStatus.summary} · ${batteryStatus.percentage}%`
            background: Rectangle {
                radius: 4
                color: batteryButton.down ? "#418bd4" : batteryButton.hovered ? "#497caf" : "transparent"
            }
            contentItem: RowLayout {
                spacing: 3
                Image {
                    Layout.preferredWidth: 19
                    Layout.preferredHeight: 19
                    source: batteryStatus.percentage < 20 ? "image://theme/battery-low" : "image://theme/battery-full"
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.PreserveAspectFit
                }
                Text {
                    Layout.fillWidth: true
                    text: `${batteryStatus.percentage}%`
                    color: "white"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Popup {
            id: batteryPopup
            x: panel.width - width - 48
            y: -height - 8
            width: 220
            height: 86
            padding: 12
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            background: Rectangle {
                radius: 7
                color: "#ffffff"
                border.color: "#9bbde0"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: batteryStatus.summary
                        color: "#141312"
                        font.bold: true
                    }
                    Text {
                        text: `${batteryStatus.percentage}%`
                        color: "#596a7c"
                    }
                }
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: batteryStatus.percentage
                }
            }
        }

        ToolButton {
            id: notificationButton
            visible: shellSettings.panelWidgets.notifications
            Layout.preferredWidth: visible ? 38 : 0
            Layout.fillHeight: true
            onClicked: notificationPopup.open()
            ToolTip.visible: hovered
            ToolTip.text: notificationModel.status
            background: Rectangle {
                radius: 4
                color: notificationButton.down ? "#418bd4" : notificationButton.hovered ? "#497caf" : "transparent"
            }
            contentItem: Item {
                Image {
                    anchors.centerIn: parent
                    width: 21
                    height: 21
                    source: "image://theme/notifications"
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.PreserveAspectFit
                }
                Rectangle {
                    visible: notificationModel.count > 0
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 1
                    anchors.topMargin: 1
                    width: 15
                    height: 15
                    radius: 8
                    color: "#c4e0ff"
                    Text {
                        anchors.centerIn: parent
                        text: notificationModel.count > 9 ? "9+" : notificationModel.count
                        color: "#141312"
                        font.pixelSize: 9
                        font.bold: true
                    }
                }
            }
        }

        Popup {
            id: notificationPopup
            x: panel.width - width - 12
            y: -height - 8
            width: 360
            height: 400
            padding: 10
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            background: Rectangle {
                radius: 7
                color: "#ffffff"
                border.color: "#9bbde0"
            }
            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Notifications"
                        color: "#141312"
                        font.bold: true
                        font.pixelSize: 15
                    }
                    Text {
                        text: notificationModel.count
                        color: "#68798a"
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: !notificationModel.active
                    text: notificationModel.status
                    color: "#68798a"
                    wrapMode: Text.Wrap
                    font.pixelSize: 11
                }
                ListView {
                    id: notificationList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 6
                    model: notificationModel
                    delegate: Rectangle {
                        required property uint notificationId
                        required property string appName
                        required property string appIcon
                        required property string summary
                        required property string body
                        required property var actions
                        width: ListView.view.width
                        height: notificationColumn.implicitHeight + 14
                        radius: 5
                        color: "#f8f7f6"
                        border.color: "#e0eaf6"
                        ColumnLayout {
                            id: notificationColumn
                            anchors.fill: parent
                            anchors.margins: 7
                            spacing: 4
                            RowLayout {
                                Layout.fillWidth: true
                                Image {
                                    visible: appIcon.length > 0
                                    Layout.preferredWidth: visible ? 18 : 0
                                    Layout.preferredHeight: visible ? 18 : 0
                                    source: appIcon.startsWith("/") ? `file://${appIcon}` : `image://theme/${encodeURIComponent(appIcon)}`
                                    sourceSize: Qt.size(18, 18)
                                    fillMode: Image.PreserveAspectFit
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: appName.length ? `${appName}: ${summary}` : summary
                                    color: "#141312"
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                ToolButton {
                                    text: "×"
                                    implicitWidth: 26
                                    implicitHeight: 24
                                    onClicked: notificationModel.dismiss(notificationId)
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: body.length > 0
                                text: body
                                textFormat: Text.PlainText
                                color: "#596a7c"
                                wrapMode: Text.Wrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                                font.pixelSize: 11
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Repeater {
                                    model: Math.floor(actions.length / 2)
                                    delegate: Button {
                                        required property int index
                                        text: actions[index * 2 + 1]
                                        onClicked: notificationModel.invokeAction(notificationId, actions[index * 2])
                                    }
                                }
                            }
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: notificationModel.count === 0
                        text: notificationModel.active ? "No notifications" : "No notification service is available"
                        color: "#6f8092"
                        font.pixelSize: 12
                    }
                }
            }
        }

        Popup {
            id: notificationToast
            x: panel.width - width - 12
            y: -height - panel.panelHeight - 10
            width: 350
            height: 112
            padding: 12
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            onClosed: notificationToastTimer.stop()
            background: Rectangle {
                radius: 7
                color: "#ffffff"
                border.color: "#6597c6"
            }
            ColumnLayout {
                anchors.fill: parent
                spacing: 5
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: panel.toastNotification.appName || "Notification"
                        color: "#68798a"
                        font.pixelSize: 10
                    }
                    ToolButton {
                        text: "×"
                        implicitWidth: 26
                        implicitHeight: 22
                        onClicked: notificationToast.close()
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: panel.toastNotification.summary || ""
                    color: "#141312"
                    font.bold: true
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: panel.toastNotification.body || ""
                    textFormat: Text.PlainText
                    color: "#596a7c"
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    font.pixelSize: 11
                }
            }
        }

        Connections {
            target: notificationModel
            function onNotificationAdded(id) {
                if (panel.targetScreen === Qt.application.screens[0]) {
                    panel.toastNotification = notificationModel.latestNotification
                    notificationToast.open()
                    notificationToastTimer.restart()
                }
            }
            function onNotificationClosed(id, reason) {
                if (panel.toastNotification.notificationId === id)
                    notificationToast.close()
            }
        }
        Timer {
            id: notificationToastTimer
            interval: 7000
            repeat: false
            onTriggered: notificationToast.close()
        }

        RowLayout {
            id: statusTray
            visible: shellSettings.panelWidgets.tray
            Layout.preferredWidth: visible ? implicitWidth : 0
            Layout.fillHeight: true
            spacing: 1
            Repeater {
                model: statusNotifierModel
                delegate: ToolButton {
                    id: trayButton
                    required property string serviceId
                    required property string title
                    required property string iconName
                    required property string iconDataUrl
                    required property string menuPath
                    required property string status
                    required property bool itemIsMenu
                    visible: status !== "Passive"
                    Layout.preferredWidth: visible ? 34 : 0
                    Layout.fillHeight: true
                    onClicked: {
                        if (menuPath.length)
                            panel.showTrayMenu(trayButton, serviceId, menuPath)
                        else if (itemIsMenu) {
                            const point = trayButton.mapToGlobal(Qt.point(width / 2, height / 2))
                            statusNotifierModel.contextMenu(serviceId, Math.round(point.x), Math.round(point.y))
                        }
                        else
                        {
                            const point = trayButton.mapToGlobal(Qt.point(width / 2, height / 2))
                            statusNotifierModel.activate(serviceId, Math.round(point.x), Math.round(point.y))
                        }
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: title
                    background: Rectangle {
                        radius: 4
                        color: trayButton.down ? "#418bd4" : trayButton.hovered ? "#497caf" : "transparent"
                    }
                    contentItem: Item {
                        Image {
                            id: trayIcon
                            anchors.centerIn: parent
                            width: 20
                            height: 20
                            sourceSize: Qt.size(width, height)
                            fillMode: Image.PreserveAspectFit
                            source: trayButton.iconDataUrl.length > 0 ? trayButton.iconDataUrl
                                  : trayButton.iconName.startsWith("/") ? `file://${trayButton.iconName}`
                                  : trayButton.iconName.length > 0 ? `image://theme/${encodeURIComponent(trayButton.iconName)}` : ""
                            visible: status === Image.Ready
                        }
                        Text {
                            anchors.centerIn: parent
                            visible: !trayIcon.visible
                            text: trayButton.title.length > 0 ? trayButton.title.substring(0, 1).toUpperCase() : "•"
                            color: "white"
                            font.pixelSize: 15
                            font.bold: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton | Qt.MiddleButton
                            onPressed: mouse => {
                                if (mouse.button === Qt.RightButton && trayButton.menuPath.length)
                                    panel.showTrayMenu(trayButton, trayButton.serviceId, trayButton.menuPath)
                                else if (mouse.button === Qt.RightButton) {
                                    const point = trayButton.mapToGlobal(Qt.point(mouse.x, mouse.y))
                                    statusNotifierModel.contextMenu(trayButton.serviceId, Math.round(point.x), Math.round(point.y))
                                }
                                else
                                {
                                    const point = trayButton.mapToGlobal(Qt.point(mouse.x, mouse.y))
                                    statusNotifierModel.secondaryActivate(trayButton.serviceId, Math.round(point.x), Math.round(point.y))
                                }
                            }
                        }
                    }
                }
            }
        }

        Popup {
            id: trayMenuPopup
            x: panel.width - width - 8
            y: -height - 7
            width: 280
            height: Math.min(440, trayMenuContent.implicitHeight + padding * 2)
            padding: 8
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            onClosed: remoteMenuModel.close()

            background: Rectangle {
                radius: 6
                color: "#f3f7fc"
                border.color: "#789bc0"
            }

            contentItem: ColumnLayout {
                id: trayMenuContent
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    visible: remoteMenuModel.depth > 1
                    ToolButton {
                        text: "‹ Back"
                        onClicked: remoteMenuModel.goBack()
                    }
                    Item { Layout.fillWidth: true }
                }

                ListView {
                    id: remoteMenuList
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(contentHeight, 380)
                    model: remoteMenuModel
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }

                    delegate: ItemDelegate {
                        id: remoteMenuItem
                        required property int itemId
                        required property string label
                        required property bool itemEnabled
                        required property bool separator
                        required property bool itemCheckable
                        required property bool itemChecked
                        required property bool submenu
                        required property string iconName
                        width: remoteMenuList.width
                        height: separator ? 9 : 34
                        enabled: !separator && itemEnabled
                        padding: 6
                        background: Rectangle {
                            color: remoteMenuItem.hovered && !remoteMenuItem.separator ? "#cde2f7" : "transparent"
                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                height: 1
                                visible: remoteMenuItem.separator
                                color: "#b2c2d2"
                            }
                        }
                        contentItem: RowLayout {
                            spacing: 8
                            Text {
                                Layout.preferredWidth: 18
                                text: remoteMenuItem.itemCheckable && remoteMenuItem.itemChecked ? "✓" : ""
                                color: "#24496d"
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Image {
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                source: remoteMenuItem.iconName.length
                                        ? `image://theme/${encodeURIComponent(remoteMenuItem.iconName)}` : ""
                                sourceSize: Qt.size(18, 18)
                                fillMode: Image.PreserveAspectFit
                                visible: status === Image.Ready
                            }
                            Text {
                                Layout.fillWidth: true
                                text: remoteMenuItem.label
                                textFormat: Text.PlainText
                                color: remoteMenuItem.itemEnabled ? "#1a2836" : "#788897"
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: remoteMenuItem.submenu
                                text: "›"
                                color: "#4b6680"
                            }
                        }
                        onClicked: {
                            remoteMenuModel.trigger(itemId)
                            if (!submenu)
                                trayMenuPopup.close()
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: remoteMenuModel.loading || remoteMenuModel.error.length > 0
                             || (!remoteMenuModel.loading && remoteMenuModel.itemCount === 0)
                    text: remoteMenuModel.loading ? "Loading menu…"
                          : remoteMenuModel.error.length ? remoteMenuModel.error : "No menu items are available."
                    color: remoteMenuModel.error.length ? "#9d2424" : "#53697e"
                    wrapMode: Text.WordWrap
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    Layout.topMargin: 6
                    Layout.bottomMargin: 6
                }
            }
        }

        ToolButton {
            id: keyboardLayoutButton
            visible: keyboardLayoutStatus.available && shellSettings.panelWidgets.keyboard
            Layout.preferredWidth: visible ? 40 : 0
            Layout.fillHeight: true
            text: keyboardLayoutStatus.currentLabel
            onClicked: keyboardLayoutStatus.nextLayout()
            ToolTip.visible: hovered
            ToolTip.text: `${keyboardLayoutStatus.currentName} · click to switch`
            background: Rectangle {
                radius: 4
                color: keyboardLayoutButton.down ? "#418bd4" : keyboardLayoutButton.hovered ? "#497caf" : "transparent"
            }
            contentItem: Text {
                text: keyboardLayoutButton.text
                color: "white"
                font.pixelSize: 11
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Timer {
            id: volumeUpdateTimer
            interval: 120
            repeat: false
            onTriggered: audioControl.setVolume(Math.round(volumeSlider.value))
        }

        ToolButton {
            id: clockButton
            visible: shellSettings.panelWidgets.clock
            Layout.preferredWidth: visible ? 92 : 0
            Layout.fillHeight: true
            onClicked: calendarPopup.open()
            ToolTip.visible: hovered
            ToolTip.text: "Open calendar"
            background: Rectangle {
                radius: 4
                color: clockButton.down ? "#418bd4" : clockButton.hovered ? "#497caf" : "transparent"
            }
            contentItem: Column {
                spacing: 1
                anchors.centerIn: parent
                Text {
                    id: clockTime
                    width: parent.width
                    color: "white"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                    text: Qt.formatTime(new Date(), "h:mm AP")
                }
                Text {
                    id: clockDate
                    width: parent.width
                    color: "#d8ebff"
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignRight
                    text: Qt.formatDate(new Date(), "ddd, MMM d")
                }
                Timer {
                    interval: 15000
                    running: true
                    repeat: true
                    onTriggered: {
                        clockTime.text = Qt.formatTime(new Date(), "h:mm AP")
                        clockDate.text = Qt.formatDate(new Date(), "ddd, MMM d")
                    }
                }
            }
        }

        Popup {
            id: calendarPopup
            x: panel.width - width - 8
            y: -height - 8
            width: 270
            height: 294
            padding: 10
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            function dateAt(index) {
                const firstDay = new Date(displayedMonth.getFullYear(), displayedMonth.getMonth(), 1)
                const mondayOffset = (firstDay.getDay() + 6) % 7
                return new Date(displayedMonth.getFullYear(), displayedMonth.getMonth(), 1 - mondayOffset + index)
            }

            background: Rectangle {
                radius: 7
                color: "#ffffff"
                border.color: "#9bbde0"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 5
                RowLayout {
                    Layout.fillWidth: true
                    ToolButton {
                        text: "‹"
                        font.pixelSize: 20
                        onClicked: panel.displayedMonth = new Date(panel.displayedMonth.getFullYear(), panel.displayedMonth.getMonth() - 1, 1)
                    }
                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: Qt.formatDate(panel.displayedMonth, "MMMM yyyy")
                        color: "#141312"
                        font.bold: true
                    }
                    ToolButton {
                        text: "›"
                        font.pixelSize: 20
                        onClicked: panel.displayedMonth = new Date(panel.displayedMonth.getFullYear(), panel.displayedMonth.getMonth() + 1, 1)
                    }
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 7
                    Repeater {
                        model: ["M", "T", "W", "T", "F", "S", "S"]
                        delegate: Text {
                            required property string modelData
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: modelData
                            color: "#68798a"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }
                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 7
                    Repeater {
                        model: 42
                        delegate: ItemDelegate {
                            required property int index
                            readonly property date calendarDate: calendarPopup.dateAt(index)
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            padding: 0
                            text: calendarDate.getDate().toString()
                            opacity: calendarDate.getMonth() === panel.displayedMonth.getMonth() ? 1.0 : 0.42
                            highlighted: calendarDate.getFullYear() === new Date().getFullYear()
                                         && calendarDate.getMonth() === new Date().getMonth()
                                         && calendarDate.getDate() === new Date().getDate()
                            onClicked: panel.displayedMonth = new Date(calendarDate.getFullYear(), calendarDate.getMonth(), 1)
                            background: Rectangle {
                                radius: 4
                                color: parent.highlighted ? "#91bce6" : parent.hovered ? "#e8f1fb" : "transparent"
                                border.color: parent.highlighted ? "#418bd4" : "transparent"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "#141312"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
        }
    }
}
