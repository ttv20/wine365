# Wine 365 Manager

Wine 365 Manager is included in the Wine 365 source tree at `tools/wine365-manager`. It manages a prebuilt Wine 365 runner, prefixes, Office shortcuts, updates, and removal.

The interface opens as a token-protected local page bound only to `127.0.0.1`. It requires Python 3 and a web browser, but no GTK, Electron, daemon, administrator access, or system-wide installation.

## Install

End users install the prebuilt Wine runner and manager together by downloading the single executable `.run` artifact from a GitHub Release and running it without root.

For manager development only, the manager can be installed without a runner from the Wine source root:

```bash
./tools/wine365-manager/install-user.sh
```

Open **Wine 365 Manager** from the application menu, or run:

```bash
~/.local/bin/wine365-manager
```

The manager defaults to:

- Wine prefix: `~/.wine365`
- User runner: `${XDG_DATA_HOME:-~/.local/share}/wine365/runner`
- Update manifest: not configured

## Features

- Create or rollback-safely recreate a selected Wine prefix.
- Create/update/remove Word, Excel, PowerPoint, and Outlook shortcuts.
- Optionally copy shortcuts to the visible Desktop.
- Open Wine Configuration, Registry Editor, Control Panel, File Manager, Command Prompt, Task Manager, and Uninstaller.
- Use the Wine runner prebuilt by GitHub Actions and delivered inside the release `.run` file.
- Stream and cancel verified update downloads.
- Download a future update manifest over HTTPS, verify the installer SHA-256, and run the verified installer.
- Remove the manager, runner, configuration, and shortcuts; prefix deletion requires a separate explicit checkbox.

## One-file Wine + manager installer

`packaging/build-onefile.sh` combines an already compiled Wine runner and this manager into one executable `.run` file:

```bash
tools/wine365-manager/packaging/build-onefile.sh \
  /path/to/compiled/runner \
  release/wine365-1.0.0-x86_64.run \
  1.0.0
```

The resulting executable supports:

```bash
./wine365-1.0.0-x86_64.run             # install for the current user
./wine365-1.0.0-x86_64.run --update    # atomically replace the runner and refresh the manager
./wine365-1.0.0-x86_64.run --version
./wine365-1.0.0-x86_64.run --extract DIRECTORY
./wine365-1.0.0-x86_64.run --uninstall # preserve ~/.wine365
```

Installation refuses root and writes below `${XDG_DATA_HOME:-~/.local/share}/wine365`. Runner replacement keeps the previous runner until the new manager installation succeeds.

## Update manifest

No update address is configured yet. The format is ready for a future HTTPS endpoint:

```json
{
  "version": "1.1.0",
  "installer_url": "https://example.invalid/wine365-1.1.0-x86_64.run",
  "sha256": "64 lowercase hexadecimal characters",
  "size": 123456789
}
```

The manifest address can later be embedded in a bundle, set in the manager, or supplied through the GitHub repository variable `WINE365_UPDATE_MANIFEST_URL`. The installer URL can be supplied through `WINE365_INSTALLER_URL`.

## GitHub CI/CD

`.github/workflows/wine365-onefile.yml`:

1. Builds the merged Wine 365 tree on Ubuntu.
2. Installs it into a staging directory.
3. Creates one executable containing Wine and the manager.
4. Uploads the `.run`, SHA-256 file, and update manifest as a CI artifact.
5. Publishes those files to a GitHub Release for tags named `wine365-v*`.

The workflow can also be run manually with an explicit version and future update URLs.

## Removal

Remove only the manager while preserving the runner, configuration, and prefix:

```bash
./tools/wine365-manager/uninstall-user.sh
```

Remove manager, runner, and configuration while preserving the prefix:

```bash
~/.local/share/wine365/bin/wine365-uninstall --purge-runner
```

The manager UI provides both removal modes and requires explicit confirmation before deleting the selected prefix.

## Tests

```bash
python3 -m unittest discover -s tools/wine365-manager/tests -v
tools/wine365-manager/tests/test_onefile.sh
```
