# Wine365

Wine365 is an experimental [Wine](https://www.winehq.org/) fork for running
modern Microsoft Word on Linux without a Windows virtual machine.

> Every Wine365 investigation, code change, test, and document was produced with
> AI, primarily **ChatGPT-5.6-sol** and **Grok 4.5**. I directed and tested the
> work, but **I do not know C** and cannot independently audit the C/C++ code.

## Important

- **Word is the only Office application that worked in the tested installation.**
- Tests ran only on **7th-generation and 12th-generation Intel hardware**.
- Licensing was tested only with an **organizational Microsoft 365 subscription**.
  KMS, MAK, VL, perpetual, retail, device, shared-computer, and other licensing
  methods remain untested.

This is a narrow experiment, not a promise that Wine365 will work with another
machine, Office edition, account, or Wine prefix.

## What worked

In the tested installation, Word can:

- install through Office Click-to-Run's Delivery Optimization path and launch
  after a fresh installation;
- expose automatic Office updates and complete an **Update Now** check;
- create, edit, save, close, and reopen DOCX files;
- use core UI, tables, Shapes, WordArt, comments, Save As, PDF export, and print
  preview;
- sign in and retain Microsoft 365 Apps for enterprise activation after restarts;
- use RTL-language input, resources, UI, and text rendering.

Word has not been exhaustively tested. These results do not cover every feature,
file type, add-in, macro, printer, or cloud service.

## Why this will not be submitted to WineHQ

WineHQ's
[Clean Room Guidelines](https://gitlab.winehq.org/wine/wine/-/wikis/Clean-Room-Guidelines)
state:

> Don't use an LLM tool to generate code. There's no guarantee that the training
> material of that LLM respects our Clean Room Guidelines, or that its output is
> compatible with the LGPL.

Wine365 was produced with AI, so it cannot meet this rule. These changes will
**not** be pushed or submitted to the official WineHQ repository.

---

## Technical details

### Base

Wine365 is based on **Wine 11.12**
([`996020f410e`](https://gitlab.winehq.org/wine/wine/-/commit/996020f410e7a1aa2dd6b44cf740854ea524d31a))
with Wine365 compatibility changes through this branch. Relative to that base,
the branch changes 151 paths with about 15,963 insertions and 715 deletions.

### Main changes

- **Startup and licensing:** SPPC behavior, App-V dependencies, COM activation,
  EnterpriseData WinRT, MSXML, RetailInfo, Windows JSON, AppCapability, and
  RichEdit APIs used by Word.
- **Graphics:** DXGI/D3D11 shared resources, KMT handles, keyed mutexes, D2D SVG,
  shape shaders, WIC target reuse, geometry fixes, and GDI+ antialiasing.
- **Wayland and RTL languages:** popup stacking, caption controls, input regions,
  XKB language reporting, and preferred-UI-language handling.
- **Installation, updates, and sign-in:** ranged BITS downloads, an
  Office-compatible Delivery Optimization service, `CoCancelCall`, COM teardown,
  WinHTTP/WinINet
  compatibility, WAM/OneAuth objects, and federated MSHTML navigation.
- **OAuth:** `wine365auth.exe` runs inside the prefix, uses PKCE, handles federated
  returns, and stores WAM state with DPAPI under
  `%LOCALAPPDATA%\Wine365\WAM` without logging tokens.

Wine's metric-compatible Tahoma fonts were extended with the RTL glyph ranges
used by Office. Font generation and licensing details are in
[`fonts/README.wine365-tahoma-hebrew.md`](fonts/README.wine365-tahoma-hebrew.md)
and [`fonts/LICENSE.Liberation`](fonts/LICENSE.Liberation).

### Delivery Optimization

Wine365 exposes Office's legacy Delivery Optimization COM interfaces through
`qmgr`, backed by Wine's BITS and WinHTTP transfer engine. It supports file and
`IStream` sinks, file/job properties, swarm statistics, and arbitrary CDN byte
ranges. COM proxy security blankets and generated 32/64-bit proxy/stub code let
Office use the service across process boundaries.

A clean test installation completed with exit code 0 after Office transferred
CABs and stream data through this path, including one request containing 2,576
ranges. Word then launched, its Account page showed automatic updates enabled,
and **Update Now** reported the installed build current. This implementation is
direct HTTP/CDN compatibility only; Windows peer discovery, peer sharing, and
full Delivery Optimization cache policy are not implemented.

### Build

Use an out-of-tree build after installing Wine's normal dependencies:

```sh
SOURCE="$PWD"
mkdir -p ../wine365-build
cd ../wine365-build
"$SOURCE/configure" --enable-archs=i386,x86_64
make -j"$(nproc)"
sudo make install
```

See WineHQ's [build guide](https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine)
for dependencies. Office, credentials, tokens, and proprietary Microsoft fonts
are not included.

### Limitations

Some implementations are minimal and Word-specific. Native behavior still needs
focused tests, diagnostics remain in the tree, and the code has not received an
independent C/C++ audit. Do not assume it is safe or correct for unrelated
Windows applications.

More details are in
[`OFFICE365-WORK-IN-PROGRESS.md`](OFFICE365-WORK-IN-PROGRESS.md).

### License

Wine365 retains Wine's GNU LGPL 2.1-or-later license. See [`LICENSE`](LICENSE)
and [`COPYING.LIB`](COPYING.LIB). Liberation Sans glyph material uses the SIL
Open Font License.

Wine365 is not affiliated with or endorsed by WineHQ or Microsoft.
