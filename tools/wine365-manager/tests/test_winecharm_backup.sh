#!/usr/bin/env bash
# Validate WineCharm runner-backup layout without building Wine.
set -euo pipefail

HERE=$(cd "$(dirname "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
RUNNER="$TMP/source-runner"
ARCHIVE="$TMP/release/wine365-test.tar.zst"
NAME=wine365-test
mkdir -p "$RUNNER/bin" "$RUNNER/lib/wine"
cat > "$RUNNER/bin/wine" <<'SH'
#!/bin/sh
echo wine-365-test
SH
chmod +x "$RUNNER/bin/wine"
ln -s wine "$RUNNER/bin/wine64"
printf 'dll' > "$RUNNER/lib/wine/test.dll"

"$HERE/packaging/build-winecharm-backup.sh" "$RUNNER" "$ARCHIVE" "$NAME" >/dev/null
[[ -f "$ARCHIVE.sha256" ]]
(cd "$(dirname "$ARCHIVE")" && sha256sum -c "$(basename "$ARCHIVE").sha256" >/dev/null)
[[ $(tar -tf "$ARCHIVE" | sed 's|/.*||' | sort -u) == "$NAME" ]]
tar -xf "$ARCHIVE" -C "$TMP"
[[ $("$TMP/$NAME/bin/wine" --version) == wine-365-test ]]
[[ -L "$TMP/$NAME/bin/wine64" ]]
[[ $(readlink "$TMP/$NAME/bin/wine64") == wine ]]
echo "WineCharm runner backup layout: PASS"
