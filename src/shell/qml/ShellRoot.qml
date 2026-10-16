/* SPDX-License-Identifier: MIT */
import QtQuick

Item {
    id: shell

    property var windows: []
    property var screenBundles: []
    property var settingsWindow: null
    property var authWindow: null

    Component {
        id: desktopComponent
        DesktopWindow { }
    }

    Component {
        id: panelComponent
        PanelWindow { }
    }

    Component {
        id: launcherComponent
        LauncherWindow { }
    }

    Component {
        id: settingsComponent
        SettingsWindow { }
    }

    Component {
        id: polkitAuthComponent
        PolkitAuthWindow { }
    }

    function findBundle(targetScreen) {
        for (const bundle of screenBundles) {
            if (bundle.screen === targetScreen)
                return bundle
        }
        return null
    }

    function updateWindowReferences() {
        const activeWindows = []
        if (settingsWindow)
            activeWindows.push(settingsWindow)
        if (authWindow)
            activeWindows.push(authWindow)
        for (const bundle of screenBundles)
            activeWindows.push(bundle.desktop, bundle.launcher, bundle.panel)
        windows = activeWindows
    }

    function addScreen(targetScreen) {
        if (!targetScreen || findBundle(targetScreen))
            return

        if (!settingsWindow) {
            settingsWindow = settingsComponent.createObject(null, { targetScreen })
        }

        const desktop = desktopComponent.createObject(null, {
            targetScreen,
            defaultWallpaperUrl: aeroDefaultWallpaperUrl
        })
        const launcher = launcherComponent.createObject(null, {
            targetScreen,
            settingsWindow
        })
        const panel = panelComponent.createObject(null, {
            targetScreen,
            launcherWindow: launcher
        })
        if (!desktop || !launcher || !panel) {
            if (desktop)
                desktop.destroy()
            if (launcher)
                launcher.destroy()
            if (panel)
                panel.destroy()
            return
        }

        launcher.panelWindow = panel
        screenBundles.push({ screen: targetScreen, desktop, launcher, panel })
        desktop.show()
        panel.show()
        ensureAuthenticationWindow()
        updateWindowReferences()
    }

    function removeScreen(targetScreen) {
        for (let i = screenBundles.length - 1; i >= 0; --i) {
            const bundle = screenBundles[i]
            if (bundle.screen !== targetScreen)
                continue

            screenBundles.splice(i, 1)
            bundle.desktop.destroy()
            bundle.launcher.destroy()
            bundle.panel.destroy()
        }
        updateWindowReferences()
    }

    function ensureAuthenticationWindow() {
        const screens = Qt.application.screens
        if (!screens.length)
            return

        const primaryBundle = findBundle(screens[0])
        if (!primaryBundle)
            return

        if (!authWindow) {
            authWindow = polkitAuthComponent.createObject(null, {
                targetScreen: screens[0],
                panelWindow: primaryBundle.panel
            })
        } else {
            authWindow.targetScreen = screens[0]
            authWindow.panelWindow = primaryBundle.panel
        }
        updateWindowReferences()
    }

    function reconcileScreens() {
        const screens = Qt.application.screens
        for (let i = screenBundles.length - 1; i >= 0; --i) {
            if (screens.indexOf(screenBundles[i].screen) < 0)
                removeScreen(screenBundles[i].screen)
        }
        for (const targetScreen of screens)
            addScreen(targetScreen)

        if (settingsWindow) {
            settingsWindow.targetScreen = screens.length ? screens[0] : null
            if (!screens.length)
                settingsWindow.hide()
        }
        ensureAuthenticationWindow()
        updateWindowReferences()
    }

    Connections {
        target: screenEvents
        function onScreensChanged() { Qt.callLater(function() { shell.reconcileScreens() }) }
    }

    Connections {
        target: notificationModel
        function onNotificationAdded(id) {
            const notification = notificationModel.latestNotification
            shellSounds.play(notification.urgency >= 2 ? "dialog-warning" : "dialog-information")
        }
    }

    Timer {
        interval: 900
        running: true
        repeat: false
        onTriggered: shellSounds.play("desktop-login")
    }

    Component.onCompleted: Qt.callLater(function() { shell.reconcileScreens() })
    Component.onDestruction: shellSounds.play("desktop-logout")
}
