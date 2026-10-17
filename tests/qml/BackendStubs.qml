import QtQuick

// UI contract doubles: no system bus, compositor, authentication, or hardware.
QtObject {
    property QtObject applicationModel: ListModel {
        property string query: ""
        Component.onCompleted: append({ name: "Test app", storageId: "test.desktop", iconName: "application-x-generic", comment: "Application" })
        function locationAvailable(location) { return true }
        function launch(id) {}
        function openLocation(location) {}
    }
    property QtObject desktopItemModel: ListModel {
        Component.onCompleted: append({ display: "Home", url: "file:///tmp", iconName: "user-home" })
        function open(url) {}
    }
    property QtObject statusNotifierModel: ListModel {
        Component.onCompleted: append({ serviceId: "org.example.Test/StatusNotifierItem", title: "Test tray item",
            iconName: "network-wireless", iconDataUrl: "", menuPath: "/Menu", status: "Active", itemIsMenu: true })
    }
    property QtObject remoteMenuModel: ListModel {
        property int depth: 0
        property int itemCount: 0
        property bool loading: false
        property string error: ""
        Component.onCompleted: append({ itemId: 1, label: "Test menu action", iconName: "application-x-generic",
            itemEnabled: true, separator: false, itemCheckable: false, itemChecked: false, submenu: false })
        function close() {}
        function openMenu(service, path) {}
    }
    property QtObject notificationModel: ListModel {
        property bool active: true
        property string status: "Ready"
        property var latestNotification: ({ summary: "", body: "", urgency: 1, id: 0 })
        signal notificationAdded(int id)
        signal notificationClosed(int id, int reason)
    }
    property QtObject audioControl: QtObject {
        property bool available: true
        property bool muted: false
        property int volume: 50
        function setVolume(value) {}
    }
    property QtObject batteryStatus: QtObject {
        property bool present: true
        property int percentage: 75
        property string summary: "75%"
    }
    property QtObject keyboardLayoutStatus: QtObject {
        property bool available: true
        property string currentLabel: "US"
        property string currentName: "English (US)"
    }
    property QtObject networkStatus: QtObject {
        property bool available: true
        property bool busy: false
        property bool connected: true
        property bool wifiEnabled: true
        property var connections: [{ name: "Test connection", uuid: "test-uuid", type: "wifi", active: true }]
        property var wifiNetworks: [{ ssid: "Test Wi-Fi", security: "WPA2", signal: 75, inUse: true }]
        property string summary: "Connected"
        property string error: ""
        function refresh() {}
        function scanWifi() {}
    }
    property QtObject powerActions: QtObject {
        property bool serviceAvailable: true
        property bool pending: false
        property bool suspendAvailable: true
        property bool rebootAvailable: true
        property bool powerOffAvailable: true
        property string error: ""
    }
    property QtObject polkitAgentModel: QtObject {
        property bool registered: true
        property string status: "Ready"
        property bool authenticationActive: false
        property var identities: ["Test user"]
        property string message: "Test authentication"
        property string prompt: "Password:"
        property bool promptEcho: false
        property bool waitingForResponse: false
        property string error: ""
    }
    property QtObject screenLockController: QtObject { property string error: "" }
    property QtObject shellSettings: QtObject {
        property int iconThemeRevision: 0
        property string fontFamily: "Noto Sans"
        property string iconTheme: "Windows 7 Aero"
        property var availableFonts: ["Noto Sans"]
        property var availableIconThemes: ["Windows 7 Aero"]
        property var panelWidgets: ({ network: true, audio: true, battery: true,
            keyboard: true, tray: true, notifications: true, clock: true })
        signal settingsChanged()
        function wallpaperUrl(screen) { return aeroDefaultWallpaperUrl }
    }
    property QtObject shellBridge: QtObject {
        property bool showingDesktop: false
        function registerSurface(window, role) {}
        function setSurfacePosition(window, position) {}
    }
    property QtObject shellActions: QtObject { signal actionRequested(string action) }
    property QtObject screenEvents: QtObject { signal screensChanged() }
    property QtObject shellSounds: QtObject { function play(event) {} }
}
