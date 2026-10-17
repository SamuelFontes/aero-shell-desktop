#!/usr/bin/env python3
"""Run the production launcher and original widgets in a disposable virtual session."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("prefix", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument("--style-probe", type=Path)
parser.add_argument("--input-helper", type=Path)
args = parser.parse_args()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)

with tempfile.TemporaryDirectory(prefix="aeroshell-native-") as temporary:
    home = Path(temporary)
    runtime = home / "runtime"
    runtime.mkdir(mode=0o700)
    config = home / ".config"
    config.mkdir()
    sentinel = config / "kdeglobals"
    sentinel.write_text("[General]\nColorScheme=DoNotChangePlasma\n")
    state = home / "session.json"
    env = {"HOME": str(home), "PATH": os.environ["PATH"], "LANG": "C.UTF-8",
           "XDG_RUNTIME_DIR": str(runtime), "AEROSHELL_PREFIX": str(args.prefix.resolve()),
           "AEROSHELL_TEST_STATE_FILE": str(state), "LIBGL_ALWAYS_SOFTWARE": "1",
           "QT_QUICK_BACKEND": "software", "QT_LOGGING_RULES": "kwin_scripting.debug=true"}
    env["QT_FORCE_STDERR_LOGGING"] = "1"
    if args.input_helper:
        apps = home / ".local/share/aeroshell/native/applications"
        apps.mkdir(parents=True)
        (apps / "org.aeroshell.validation.desktop").write_text(
            "[Desktop Entry]\nType=Application\nName=AeroShell validation\nNoDisplay=true\n"
            f"Exec={args.input_helper.resolve()}\nX-KDE-Wayland-Interfaces=org_kde_kwin_fake_input\n")
    with (output / "session.log").open("w") as log:
        process = subprocess.Popen(["python3", str(ROOT / "session/aeroshell-native-session"), "--virtual"],
                                   env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            deadline = time.monotonic() + 55
            while not state.exists() and process.poll() is None and time.monotonic() < deadline:
                time.sleep(0.2)
            if not state.exists():
                raise RuntimeError("production launcher did not start; see session.log")
            client_env = json.loads(state.read_text())
            if args.style_probe:
                style = subprocess.run([str(args.style_probe.resolve())], env=client_env,
                                       capture_output=True, text=True, timeout=15)
                (output / "application-style.txt").write_text(style.stdout + style.stderr)
                assert style.returncode == 0, "original Kvantum application style did not load"

            def dbus(*arguments):
                return subprocess.run(["qdbus6", *arguments], env=client_env, text=True,
                                      stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)

            script = 'print(JSON.stringify(panels().map(p => ({height:p.height,widgets:p.widgets().map(w=>w.type)}))));'
            while time.monotonic() < deadline and process.poll() is None:
                result = dbus("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell.evaluateScript", script)
                if result.returncode == 0 and "SevenStart" in result.stdout:
                    break
                time.sleep(0.5)
            else:
                raise RuntimeError("original panel failed to load; see session.log")
            (output / "widgets.json").write_text(result.stdout)
            shell_pid = int(dbus("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                 "org.freedesktop.DBus.GetConnectionUnixProcessID", "org.kde.plasmashell").stdout)
            maps = Path(f"/proc/{shell_pid}/maps").read_text()
            assert str(args.prefix.resolve() / "lib/libPlasmaQuick.so") in maps, "shell loaded stock PlasmaQuick"
            (output / "private-libraries.txt").write_text("\n".join(line for line in maps.splitlines() if "libPlasma" in line))
            for widget in ("SevenStart", "seventasks", "systemtray", "digitalclocklite", "win7showdesktop"):
                assert widget in result.stdout, widget
            effects = dbus("org.kde.KWin", "/Effects", "org.freedesktop.DBus.Properties.Get", "org.kde.kwin.Effects", "loadedEffects")
            (output / "effects.txt").write_text(effects.stdout + effects.stderr)
            assert "aeroglassblur" in effects.stdout, effects.stderr
            time.sleep(2)
            capture = subprocess.run(["spectacle", "-b", "-f", "-n", "-o", str(output / "desktop.png")],
                                     env=client_env, capture_output=True, text=True, timeout=15)
            assert capture.returncode == 0, capture.stderr
            opened = dbus("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell.activateLauncherMenu")
            assert opened.returncode == 0, opened.stderr
            time.sleep(1)
            subprocess.run(["spectacle", "-b", "-f", "-n", "-o", str(output / "start.png")],
                           env=client_env, capture_output=True, timeout=15, check=True)
            if args.input_helper:
                # The original menu opens its search field with keyboard focus.
                # Type 'dolphin' using Linux input key codes.
                subprocess.run([str(args.input_helper.resolve()), "keys", "32", "24", "38", "25", "35", "23", "49"],
                               env=client_env, capture_output=True, timeout=5, check=True)
                time.sleep(2)
                subprocess.run(["spectacle", "-b", "-f", "-n", "-o", str(output / "search.png")],
                               env=client_env, capture_output=True, timeout=15, check=True)
            if args.input_helper:
                subprocess.run([str(args.input_helper.resolve()), "keys", "28"],
                               env=client_env, capture_output=True, timeout=5, check=True)
                application_pid = None
                for attempt in range(30):
                    names = dbus("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus.ListNames").stdout.splitlines()
                    dolphin = next((name for name in names if name.startswith("org.kde.dolphin")), None)
                    if dolphin:
                        application_pid = int(dbus("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus.GetConnectionUnixProcessID", dolphin).stdout)
                        break
                    time.sleep(0.5)
                assert application_pid, "the original search result did not launch Dolphin"
            else:
                dbus("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell.activateLauncherMenu")
                application = subprocess.Popen(["dolphin", "--new-window", str(home)], env=client_env,
                                               stdout=log, stderr=subprocess.STDOUT)
                application_pid = application.pid
            time.sleep(3)
            app_maps = Path(f"/proc/{application_pid}/maps").read_text()
            assert "libkvantum.so" in app_maps, "Qt Widgets did not load Kvantum"
            support = dbus("org.kde.KWin", "/KWin", "org.kde.KWin.supportInformation")
            (output / "kwin-support.txt").write_text(support.stdout)
            assert "org.smod.smod" in support.stdout, "SMOD decoration did not load"
            subprocess.run(["spectacle", "-b", "-f", "-n", "-o", str(output / "application.png")],
                           env=client_env, capture_output=True, timeout=15, check=True)
            os.kill(application_pid, signal.SIGTERM)
            if args.input_helper:
                subprocess.run([str(args.input_helper.resolve()), "1210", "700"], env=client_env,
                               capture_output=True, timeout=5, check=True)
                time.sleep(1)
                subprocess.run(["spectacle", "-b", "-f", "-n", "-o", str(output / "calendar.png")],
                               env=client_env, capture_output=True, timeout=15, check=True)
                subprocess.run([str(args.input_helper.resolve()), "700", "300"], env=client_env,
                               capture_output=True, timeout=5, check=True)
            assert sentinel.read_text() == "[General]\nColorScheme=DoNotChangePlasma\n"
            private = config / "aeroshell/native"
            assert (private / "plasma-io.gitgud.wackyideas.desktop-appletsrc").exists()
            (output / "profile-files.txt").write_text("\n".join(str(p.relative_to(home)) for p in home.rglob("*") if p.is_file()))
            logout = dbus("org.kde.Shutdown", "/Shutdown", "org.kde.Shutdown.logout")
            assert logout.returncode == 0, logout.stderr
            process.wait(timeout=20)
            assert sentinel.read_text() == "[General]\nColorScheme=DoNotChangePlasma\n"
            print("PASS: production launcher, original widgets/private library, Aero Glass, SMOD, Kvantum, separate Plasma configuration")
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
            # Some KDE services detach from the supervisor. Reap only processes
            # whose environment identifies this disposable home and private bus.
            for proc in Path("/proc").glob("[0-9]*"):
                try:
                    environ = (proc / "environ").read_bytes().split(b"\0")
                    if (b"HOME=" + str(home).encode()) in environ and (b"AEROSHELL_PREFIX=" + str(args.prefix.resolve()).encode()) in environ:
                        os.kill(int(proc.name), signal.SIGTERM)
                except (OSError, ProcessLookupError):
                    pass
            time.sleep(1)
            # Private portal activation may create a temporary FUSE mount.
            if (runtime / "doc").is_mount():
                subprocess.run(["fusermount3", "-u", str(runtime / "doc")], check=True)
