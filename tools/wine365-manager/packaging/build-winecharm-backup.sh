#!/usr/bin/env bash
# Create a WineCharm-compatible runner backup from an already installed Wine tree.
set -euo pipefail

usage() {
    echo "Usage: $0 RUNNER_DIR OUTPUT.tar.zst RUNNER_NAME" >&2
    exit 2
}
[[ $# -eq 3 ]] || usage
RUNNER=$(cd "$1" && pwd)
OUTPUT=$2
RUNNER_NAME=$3

[[ -x "$RUNNER/bin/wine" ]] || { echo "Runner has no executable bin/wine: $RUNNER" >&2; exit 1; }
[[ $OUTPUT == *.tar.zst ]] || { echo "Output must end in .tar.zst: $OUTPUT" >&2; exit 1; }
[[ $RUNNER_NAME =~ ^[A-Za-z0-9][A-Za-z0-9._+-]{0,127}$ ]] || {
    echo "Unsafe WineCharm runner name: $RUNNER_NAME" >&2
    exit 1
}
command -v zstd >/dev/null || { echo "zstd is required" >&2; exit 1; }
mkdir -p "$(dirname "$OUTPUT")"
OUTPUT=$(cd "$(dirname "$OUTPUT")" && pwd)/$(basename "$OUTPUT")

# WineCharm restore extracts directly into its Runners directory and validates
# that the archive contains bin/wine. Rename only archive member paths; preserve
# symlink and hard-link targets exactly as installed in the Wine runner.
tar --zstd -cf "$OUTPUT" \
    --transform="flags=r;s|^\\.|$RUNNER_NAME|" \
    -C "$RUNNER" .

mapfile -t roots < <(tar -tf "$OUTPUT" | sed 's|^\./||; s|/.*||' | sed '/^$/d' | sort -u)
[[ ${#roots[@]} -eq 1 && ${roots[0]} == "$RUNNER_NAME" ]] || {
    printf 'Archive has unexpected top-level entries: %s\n' "${roots[*]:-none}" >&2
    exit 1
}
tar -tf "$OUTPUT" | grep -Fx "$RUNNER_NAME/bin/wine" >/dev/null || {
    echo "Archive does not contain $RUNNER_NAME/bin/wine" >&2
    exit 1
}
SHA256=$(sha256sum "$OUTPUT" | awk '{print $1}')
printf '%s  %s\n' "$SHA256" "$(basename "$OUTPUT")" > "$OUTPUT.sha256"
printf 'Created WineCharm runner backup: %s\nSHA-256: %s\n' "$OUTPUT" "$SHA256"
