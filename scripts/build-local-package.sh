#!/usr/bin/env bash
# Package the current workspace, including uncommitted implementation changes.
set -Eeuo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="${1:-$root/build/package-current}"
mkdir -p -- "$output"
output="$(realpath "$output")"
cd "$root"
archive="$output/aeroshell-desktop-source.tar.gz"
tar --transform 's,^,aeroshell/,' -czf "$archive" CMakeLists.txt LICENSE README.md plan.md assets cmake defaults docs packaging patches scripts session src tests
checksum="$(sha256sum "$archive")"
checksum="${checksum%% *}"
version="0.1.0.r$(git rev-list --count HEAD).g$(git rev-parse --short HEAD).fix$(date +%Y%m%d%H%M%S)"
python - "$root/packaging/arch/PKGBUILD" "$output/PKGBUILD" "$checksum" "$version" <<'PY'
from pathlib import Path
import re
import sys
source, target, checksum, version = sys.argv[1:]
text = Path(source).read_text()
text = text.replace('pkgname=aeroshell-desktop-git', 'pkgname=aeroshell-desktop')
text = re.sub(r'^pkgver=.*$', 'pkgver=' + version, text, flags=re.M)
text = text.replace("conflicts=('aeroshell-desktop')", "conflicts=('aeroshell-desktop-git')")
text = re.sub(r'^source=.*$', "source=('aeroshell-desktop-source.tar.gz')", text, flags=re.M)
text = re.sub(r'^sha256sums=.*$', "sha256sums=('" + checksum + "')", text, flags=re.M)
text = re.sub(r'\npkgver\(\) \{.*?\n\}\n', '\n', text, flags=re.S)
text = text.replace('"${srcdir}/${pkgname}"', '"${srcdir}/aeroshell"')
text += "\noptions=('!debug')\nCOMPRESSZST=(zstd -c -T2 -10 -)\n"
Path(target).write_text('# Local workspace snapshot; source archive checksum is pinned.\n' + text)
PY
cd "$output"
if [[ "${AEROSHELL_PACKAGE_PREPARE_ONLY:-0}" == 1 ]]; then
    printf 'Prepared workspace source archive and PKGBUILD in %s\n' "$output"
    exit 0
fi
CMAKE_BUILD_PARALLEL_LEVEL=4 makepkg --noconfirm --cleanbuild
sha256sum ./*.pkg.tar.zst > package.sha256
