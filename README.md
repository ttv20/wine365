# Wine365

Wine365 is an experimental fork of [Wine](https://www.winehq.org/) created to
make modern Microsoft Word run on Linux without a Windows virtual machine.

> **Important:** Every Wine365 investigation, code change, test workflow, and
> document was produced with AI. I directed the work and tested the results, but
> **I do not know C** and cannot independently audit the C/C++ implementation.

## Read this first

- **Everything added for Wine365 was done with AI**, primarily
  **ChatGPT-5.6-sol** and **Grok 4.5**. The upstream Wine code is not included in
  that statement.
- **I do not know C.** I chose the goals, supplied test environments, and
  evaluated results, while AI inspected, wrote, debugged, and documented the
  implementation.
- **Microsoft Word is the only Office application that worked in the tested
  installation.** No claim is made that Excel, PowerPoint, Outlook, OneNote, or
  any other Office application works.
- Testing has been performed only on systems with **7th-generation and
  12th-generation Intel hardware**. AMD systems, other Intel generations,
  NVIDIA graphics, and other hardware combinations have not been validated.
- Licensing has been tested only with an **organizational Microsoft 365
  subscription**. KMS, MAK, Volume Licensing (VL), perpetual/retail licenses,
  device-based activation, shared-computer activation, and every other licensing
  method remain untested.

This repository is a record of an experiment that worked in a narrow tested
environment. It is not a promise that the same result will work on another
machine, Office edition, account, or Wine prefix.

## What worked

In the validated installation, Microsoft Word can:

- launch after a fresh Microsoft Office installation;
- create, edit, save, close, and reopen DOCX documents;
- use core UI, tables, Shapes, modern WordArt, comments, Save As, PDF export,
  and print preview;
- sign in with an organizational account and retain Microsoft 365 Apps for
  enterprise activation across Word and container restarts;
- switch between English and Hebrew input, start with Hebrew-only Office
  resources, render an RTL interface, and display Hebrew correctly.

Word has not been exhaustively tested. Passing these workflows does not imply
that every Word feature, document type, add-in, macro, printer, or cloud service
works.

## Why this will not be submitted to WineHQ

WineHQ's official
[Clean Room Guidelines](https://gitlab.winehq.org/wine/wine/-/wikis/Clean-Room-Guidelines)
explicitly state:

> Don't use an LLM tool to generate code. There's no guarantee that the training
> material of that LLM respects our Clean Room Guidelines, or that its output is
> compatible with the LGPL.

All Wine365 implementation work was produced with AI. Its provenance therefore
cannot satisfy this WineHQ clean-room rule, so these changes **will not be pushed
or submitted to the official WineHQ repository**. Wine365 will remain a separate
experimental fork.

---

## Technical details

The sections below describe the implementation. They are intentionally placed
after the project status, authorship, test scope, and licensing limitations.

### Source baseline and size

The branch is based on **Wine 11.12**
([`996020f410e`](https://gitlab.winehq.org/wine/wine/-/commit/996020f410e7a1aa2dd6b44cf740854ea524d31a))
and contains the compatibility work through `c9060040bfb`.

Compared with Wine 11.12, the committed branch changes 137 paths with
approximately 14,581 insertions and 476 deletions. It is a checkpointed research
tree, not an upstream-ready patch series.

### Startup, App-V, and licensing internals

- Added the SPPC behavior, license/property handling, cryptographic plumbing,
  and tests required by the targeted subscription installation.
- Added App-V and Windows API dependencies reached during Click-to-Run startup,
  including package-family lookup, generic AVL operations, file/path APIs, and
  runtime exports.
- Corrected COM activation-filter handling that prevented Office from obtaining
  `IGlobalOptions` and caused Word startup error 31.
- Added the minimal EnterpriseData `ProtectionPolicyManager` WinRT behavior
  required to pass Office's protection-policy startup check.
- Filled gaps in MSXML/XPath/schema, RetailInfo, Windows JSON, AppCapability,
  RichEdit TOM, and other APIs reached by Word.

These changes were validated only against the tested Microsoft 365 subscription
path. Their presence does not indicate that KMS, MAK, VL, perpetual, retail,
device, or shared-computer licensing works.

### Graphics and document UI internals

- Implemented the DXGI/D3D11 shared-resource path used by Word: server-backed
  KMT handles, keyed mutexes, shared metadata, and pixel synchronization.
- Added the D2D SVG object subset reached by Word's startup and icon paths.
- Corrected D2D command-list completion, bounded pathological Bezier work, fixed
  self-intersecting outline joins, and embedded required shape shaders.
- Reused small WIC render targets and synchronized external D2D devices to avoid
  gallery and menu stalls.
- Added antialiasing for smoothed GDI+ path fills/outlines and RichEdit
  DirectWrite/D2D drawing used by Office text surfaces.

### Wayland windows and input internals

- Corrected GPU-client surface placement, popup ownership, z-order, and
  late-present restacking so Word galleries and sign-in dialogs remain visible.
- Preserved NUI dialog close controls and added caption-button bounds and hit
  testing for minimize, maximize, restore, and close.
- Fixed full-window input regions and focus-confinement behavior.
- Reported active XKB languages through Wine HKLs and
  `WM_INPUTLANGCHANGE`, allowing Word to distinguish English and Hebrew input.
- Implemented process/thread preferred-UI-language storage and fallback behavior
  so a Hebrew-only Office installation can start without en-US resources.

### Installation, networking, and OAuth internals

- Added BITS `AddFileWithRanges` download and progress handling required by the
  tested Click-to-Run installer.
- Implemented `CoCancelCall` and avoided blocking COM callback-interface teardown,
  removing an installer deadlock.
- Added WinHTTP option queries and WinINet legacy timeout compatibility used by
  OneAuth home-realm discovery.
- Corrected federated MSHTML form navigation so SAML POSTs and BrokerPlugin
  redirects survive the embedded sign-in flow.
- Expanded `windows.security.authentication.onlineid` with the WAM account,
  token, async-result, property-map, cached-account, silent-acquisition, account
  affinity, and OnlineId licensing objects consumed by Word's OneAuth path.

The branch also adds `wine365auth.exe`, an in-prefix OAuth broker owned by the
Word window. It hosts Microsoft authentication in MSHTML, uses authorization-code
PKCE, handles federated SAML return navigation, and exchanges the resulting code.
WAM access, ID, licensing, and refresh state is stored under
`%LOCALAPPDATA%\Wine365\WAM` using DPAPI. The broker does not open a host browser
or log tokens.

### Hebrew font internals

Wine's metric-compatible Tahoma regular and bold faces were extended with only
the Hebrew glyph and layout ranges from Liberation Sans. This covers both GDI
and Word's direct DirectWrite glyph queries without redistributing Microsoft
fonts.

Provenance, generation details, and the SIL Open Font License are recorded in
[`fonts/README.wine365-tahoma-hebrew.md`](fonts/README.wine365-tahoma-hebrew.md)
and [`fonts/LICENSE.Liberation`](fonts/LICENSE.Liberation).

### Build

Install Wine's normal build dependencies, then use an out-of-tree build:

```sh
SOURCE="$PWD"
mkdir -p ../wine365-build
cd ../wine365-build
"$SOURCE/configure" --enable-archs=i386,x86_64
make -j"$(nproc)"
```

Install with:

```sh
sudo make install
```

See WineHQ's [Building Wine](https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine)
page for distribution-specific dependencies and other configuration options.
Office, Microsoft credentials, tokens, and proprietary Microsoft fonts are not
included in this repository.

### Known implementation limitations

Several checkpoint changes remain broader or less complete than normal
Wine-quality implementations. Some APIs are minimal Word-specific behavior,
some native behavior still needs focused conformance tests, and short targeted
diagnostics remain in the tree. The code has not received an independent expert
C/C++ audit and should not be assumed safe or correct for unrelated Windows
applications.

Detailed causal findings, retained diagnostics, review issues, and test
constraints are recorded in
[`OFFICE365-WORK-IN-PROGRESS.md`](OFFICE365-WORK-IN-PROGRESS.md).

### License and upstream

Wine365 retains Wine's GNU Lesser General Public License, version 2.1 or later.
See [`LICENSE`](LICENSE) and [`COPYING.LIB`](COPYING.LIB). The appended Liberation
Sans glyph material is separately covered by the SIL Open Font License.

Wine is a separate project. Wine365 is not affiliated with or endorsed by
WineHQ or Microsoft.
