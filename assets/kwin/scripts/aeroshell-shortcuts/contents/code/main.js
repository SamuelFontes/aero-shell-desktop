/* SPDX-License-Identifier: MIT */
function shell(action) {
    callDBus("org.aeroshell.Shell", "/Shell", "org.aeroshell.Shell", "dispatch", action);
}
function shortcut(id, keys, action) {
    registerShortcut("AeroShell " + id, id, keys, function () { shell(action); });
}
shortcut("Start", "Ctrl+Esc", "start");
shortcut("Security options", "Ctrl+Alt+Delete", "security");
shortcut("Find files", "Meta+F", "file-search");
shortcut("Screenshot", "Print", "screenshot");
shortcut("Window screenshot", "Alt+Print", "window-screenshot");
shortcut("Explorer", "Meta+E", "explorer");
shortcut("Run", "Meta+R", "run");
shortcut("Lock", "Meta+L", "lock");
shortcut("Task Manager", "Ctrl+Shift+Esc", "task-manager");
shortcut("System properties", "Meta+Pause", "settings");
shortcut("Taskbar focus", "Meta+T", "taskbar");
shortcut("Notification area focus", "Meta+B", "tray");
for (let number = 1; number <= 9; ++number) {
    shortcut("Task " + number, "Meta+" + number, "task:" + number);
}
let minimized = [];
function minimize(exceptActive) {
    for (const window of workspace.windowList()) {
        if (window.normalWindow && window.minimizable && !window.minimized
            && (!exceptActive || window !== workspace.activeWindow)) {
            if (minimized.indexOf(window) === -1) minimized.push(window);
            window.minimized = true;
        }
    }
}
registerShortcut("AeroShell Minimize all", "Minimize all windows", "Meta+M", function () { minimize(false); });
registerShortcut("AeroShell Restore all", "Restore minimized windows", "Meta+Shift+M", function () {
    for (const window of minimized) {
        if (workspace.windowList().indexOf(window) !== -1) window.minimized = false;
    }
    minimized = [];
});
let otherMinimized = [];
registerShortcut("AeroShell Minimize others", "Minimize other windows", "Meta+Home", function () {
    if (otherMinimized.length) {
        for (const window of otherMinimized) if (workspace.windowList().indexOf(window) !== -1) window.minimized = false;
        otherMinimized = [];
    } else {
        otherMinimized = workspace.windowList().filter(function(window) {
            return window.normalWindow && window.minimizable && !window.minimized && window !== workspace.activeWindow;
        });
        for (const window of otherMinimized) window.minimized = true;
    }
});
function cycleWindows(reverse) {
    const windows = workspace.windowList().filter(function(window) { return window.normalWindow && !window.skipSwitcher; });
    if (!windows.length) return;
    const current = windows.indexOf(workspace.activeWindow);
    const next = windows[(current + (reverse ? windows.length - 1 : 1)) % windows.length];
    next.minimized = false;
    workspace.activeWindow = next;
}
registerShortcut("AeroShell Cycle windows", "Cycle windows in opening order", "Alt+Escape", function() { cycleWindows(false); });
registerShortcut("AeroShell Cycle windows backwards", "Cycle windows backwards", "Alt+Shift+Escape", function() { cycleWindows(true); });

registerShortcut("AeroShell Maximize", "Maximize window", "Meta+Up", function () {
    const window = workspace.activeWindow;
    if (window && window.normalWindow && window.maximizable) window.setMaximize(true, true);
});
registerShortcut("AeroShell Restore or minimize", "Restore or minimize window", "Meta+Down", function () {
    const window = workspace.activeWindow;
    if (!window || !window.normalWindow) return;
    if (window.maximizeMode !== 0) window.setMaximize(false, false);
    else if (window.minimizable) window.minimized = true;
});
