#!/usr/bin/env bash
# Remove Wine 365 from the current user. Prefix deletion requires an explicit option.
set -euo pipefail

DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}
CONFIG_HOME=${XDG_CONFIG_HOME:-$HOME/.config}
ROOT=${WINE365_MANAGER_HOME:-$DATA_HOME/wine365}
BIN_HOME=${WINE365_BIN_HOME:-$HOME/.local/bin}
PURGE_RUNNER=false
REMOVE_PREFIX=

usage() {
    echo "Usage: $0 [--purge-runner] [--remove-prefix PATH]" >&2
}
while [[ $# -gt 0 ]]; do
    case $1 in
        --purge-runner) PURGE_RUNNER=true; shift ;;
        --remove-prefix)
            [[ $# -ge 2 ]] || { usage; exit 2; }
            REMOVE_PREFIX=$2; shift 2 ;;
        *) usage; exit 2 ;;
    esac
done

# Validate and stop the selected prefix while the installed backend is available.
if [[ -n $REMOVE_PREFIX ]]; then
    [[ -f "$ROOT/lib/wine365_backend.py" ]] || {
        echo "Cannot safely validate the prefix because the installed backend is missing." >&2
        exit 1
    }
    REMOVE_PREFIX=$(PYTHONPATH="$ROOT/lib" python3 - "$REMOVE_PREFIX" <<'PY'
import sys
import wine365_backend as backend
print(backend.validate_prefix(sys.argv[1]))
PY
)
    PYTHONPATH="$ROOT/lib" python3 - "$REMOVE_PREFIX" <<'PY' || true
import sys
import wine365_backend as backend
config = backend.load_config()
try:
    backend.stop_wine(sys.argv[1], config["wine"])
except (FileNotFoundError, OSError):
    pass
PY
fi

if [[ -f "$ROOT/lib/wine365_backend.py" ]]; then
    PYTHONPATH="$ROOT/lib" python3 - <<'PY' || true
import wine365_backend as backend
backend.remove_app_shortcuts(backend.APP_META)
PY
fi
for file in "$DATA_HOME/applications/wine365-manager.desktop" "${XDG_DESKTOP_DIR:-$HOME/Desktop}/wine365-manager.desktop"; do
    if [[ -f "$file" ]] && grep -q '^X-Wine365-Managed=true$' "$file"; then rm -f "$file"; fi
done
for link in "$BIN_HOME/wine365-manager" "$BIN_HOME/wine365-launcher"; do
    if [[ -L "$link" ]] && [[ $(readlink "$link") == "$ROOT"/bin/* ]]; then rm -f "$link"; fi
done

if [[ -n $REMOVE_PREFIX ]]; then
    rm -rf -- "$REMOVE_PREFIX"
    printf 'Removed Wine environment: %s\n' "$REMOVE_PREFIX"
fi
rm -rf "$ROOT/lib" "$ROOT/icons"
rm -f "$ROOT/bin/wine365-manager" "$ROOT/bin/wine365-launcher"
if $PURGE_RUNNER; then
    rm -rf "$ROOT/runner"
    rm -f "$ROOT/VERSION" "$ROOT/UPDATE_URL" "$ROOT/install.json"
    rm -rf "$CONFIG_HOME/wine365"
fi
rm -f "$ROOT/bin/wine365-uninstall"
rmdir "$ROOT/bin" "$ROOT" 2>/dev/null || true

if $PURGE_RUNNER; then
    echo "Wine 365 runner, manager, shortcuts, and configuration removed."
else
    printf 'Wine 365 Manager removed. Prefixes, configuration, and any runner in %s/runner were preserved.\n' "$ROOT"
fi
