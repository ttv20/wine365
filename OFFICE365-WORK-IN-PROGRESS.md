# Microsoft Office Click-to-Run compatibility snapshot

This branch is an experimental Wine 11.12 snapshot used to investigate Microsoft
Office Click-to-Run (C2R), initially Microsoft Word 2024.  It intentionally
contains work-in-progress compatibility implementations, native-behaviour tests,
and temporary tracing.  It is not an upstream-ready patch series.

## Current result

In a disposable copy of the Office prefix, Word now survives the two previously
fatal startup predicates:

1. Word error 31, caused by failed `CLSID_GlobalOptions` activation.
2. Office fail-fast `0x01483052`, caused by a missing WinRT activation factory.

The Word process now reaches a **usable experimental UI**. Wine transports
Office's three shared D3D11 surfaces correctly, and a minimal NULL-stream SVG
document/root/child implementation removes the later `0x1e3c3840` fail-fast.
The blank visible top-level was then localized to Wine's X11 offscreen
client-surface compositor: Word rendered a genuine 1440×899 start page/editor in
the redirected GL surface, but the existing GDI path did not expose it in the
1080×675 host X window. A new XRender scaling/composition path displays it.

In a disposable prefix, sign-in was dismissed, **Blank document** was clicked,
and the exact text `Wine 365 XRender interaction test` appeared in the document
with a visible caret and five-word count. SVG attributes and drawing remain
stubbed, so some icon fidelity may still be incomplete. The XRender change also
needs broad regression review and an integrated dual-architecture build. Excel
and PowerPoint have not been investigated.

No Office binary predicate is patched in the successful test.  Earlier binary
patches were used only in disposable prefixes to prove causality.

## Hebrew-only Office language and glyph fixes

A clean Hebrew Click-to-Run installation contains localized `Office16/1037`
resources but no `Office16/1033` resources.  It installed successfully, then Word
exited normally with status 0 during startup.  Adding the official en-US language
pack made Word remain open, which isolated the failure to Wine's preferred UI
language handling rather than activation, ECS, or a crash.

Wine previously returned its default `en-US` locale from all process/thread
preferred-UI-language getters while both corresponding setters were success-only
stubs.  `dlls/ntdll/locale.c` now stores canonical locale-name MULTI_SZ lists for
the process and current thread, converts name and language-ID forms, and makes the
getters honor the stored values.  Thread state falls back to process state and
then to the existing default locale.  Per-thread storage is released during
thread shutdown.  With this implementation, the same isolated Hebrew prefix
continues running for more than 60 seconds after all 453 `Office16/1033` fallback
files are removed again.

A separate rendering defect produced square glyphs throughout the Hebrew UI,
including Word's search box.  Targeted GDI tracing and a Win32 glyph probe showed
that Office selects Tahoma and every tested Hebrew `GetGlyphIndicesW()` result was
`0xffff`.  Wine's bundled metric-compatible Tahoma has no Hebrew outlines.
`FontLink` loaded a Hebrew font but could not satisfy Office's glyph-index check.
Wine also tried an installed original family before its configured
`FontSubstitutes` replacement, making a Tahoma replacement ineffective for
`DEFAULT_CHARSET`.

`dlls/win32u/font.c` now gives an explicit font substitution priority over the
original family.  `wine.inf` maps Tahoma to Liberation Sans, and the Wine 365
runtime bundles the Liberation Sans faces in its private Wine font directory.
The glyph probe then returns real glyph indices (`0507,0509,0514,0505,0519`), and
the full Word UI, including the search placeholder, renders readable Hebrew.

## Causal startup findings

### Word error 31 and App-V virtual COM

Word's startup predicate initializes COM and requests:

- CLSID `{0000034b-0000-0000-c000-000000000046}` (`CLSID_GlobalOptions`)
- IID `{0000015b-0000-0000-c000-000000000046}` (`IID_IGlobalOptions`)

It then queries and sets `COMGLB_EXCEPTION_HANDLING` to
`COMGLB_EXCEPTION_DONOT_HANDLE_ANY`.  The failing activation returned
`REGDB_E_CLASSNOTREG` (`0x80040154`), which WWLIB maps directly to startup status
31.

App-V detours `CoCreateInstance` and `CoCreateInstanceEx`.  Once the request
reaches Wine, App-V's registered `IActivationFilter` returns `S_OK` while changing
the exact `CLSID_GlobalOptions` request to `CLSID_NULL`.  The narrow compatibility
experiment in `dlls/combase/combase.c` preserves the original CLSID only for this
exact `CLSID_GlobalOptions` to `CLSID_NULL` pair.  Other successful activation
filter replacements retain normal semantics.  Word then naturally calls
`IGlobalOptions::Query` and `IGlobalOptions::Set`, and error 31 disappears.

The same file also contains broad temporary logging for COM activation,
TreatAs/filter results, and GlobalOptions calls.  This logging should be removed
when the compatibility rule has focused tests and a settled design.

### EnterpriseData WinRT activation

After error 31 was removed, first-chance C++ exception tracing identified
`winrt::hresult_class_not_registered`, followed by Office's deliberate fail-fast.
The immediate failed request was:

```
RoGetActivationFactory(
    L"Windows.Security.EnterpriseData.ProtectionPolicyManager",
    {b68f9a8c-39e0-4649-b2e4-070ab8a579b3})
```

The IID is `IProtectionPolicyManagerStatics2`.  The experimental implementation in
`dlls/combase/roapi.c` supplies that interface and returns `FALSE` with `S_OK` from
`IsProtectionEnabled`.  Office calls it three times and continues.

This implementation is deliberately minimal.  Most other methods are stubs, the
ABI is declared locally, and the runtime class is special-cased in combase.  A
proper implementation should add the interfaces and runtime class metadata to
`windows.security.enterprisedata.idl`, implement a conventional
`windows.security.enterprisedata.dll`, and add conformance tests.

## Rendering progress: genuine shared keyed textures

Office creates exactly three relevant 1536×1024 BGRA8 textures with
render-target/shader-resource bindings and
`D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX` (`MiscFlags == 0x100`). The current dirty
follow-up replaces the earlier null-handle diagnostic with:

- a private DXGI-to-D3D11 sharing bridge;
- genuine Wine-server-backed KMT resources and keyed mutexes;
- stable nonzero legacy KMT handles;
- validated legacy `ID3D11Device::OpenSharedResource` support;
- fixed-width pointer-free metadata usable by i386 and x86_64;
- a named shared-memory canonical pixel payload;
- per-wrapper local textures synchronized by staging readback/upload at keyed
  ownership transitions.

Office uses the expected producer/consumer sequence: the owner acquires key 0 and
releases key 1; the opened consumer acquires key 1 with timeout 0 and releases key
0. Diagnostic tracing verified all three pixel paths. Payload generations advance,
producer checksums change as Word draws, consumer upload checksums match each
published generation, both row pitches are 6144 bytes, and no transfer failure is
reported.

The experimental SVG implementation advances that sequence as follows:

```text
CreateSvgDocument(NULL, {850,161}, ...) -> S_OK
GetRoot(...) -> non-NULL `svg` element
CreateChild(L"path", ...) -> S_OK (repeated)
DrawSvgDocument(...) reached
```

This removes the debugger/fail-fast result, but the SVG objects do not retain the
requested attributes (`viewBox`, `width`, `height`, `preserveAspectRatio`, `fill`,
`fill-opacity`, and path `d`) and `DrawSvgDocument` remains a stub. The focused
`wine365:kmt-svg-child` run therefore kept Word alive without Program Error but
its visible main HWND remained blank. Direct X capture subsequently proved that
the redirected 1440×899 GL client surface already contained Word's genuine UI.

The X11 compositor calls `NtGdiStretchBlt()` after each offscreen GL swap. It
reported success while failing to expose those pixels in the 1080×675 host X
window. Clipping changes, equal-size copies, direct X copies, named XComposite
pixmaps, reparenting, and a forced wined3d GDI present did not fix the visible
result. An `XGetImage`/nearest-neighbor/`XPutImage` diagnostic proved that reading,
scaling, and writing the source did work once a trailing GDI blit stopped
overwriting it.

The current X11 follow-up replaces that expensive CPU loop with XRender source
and destination pictures, a source transform for DPI scaling, bilinear filtering,
and `PictOpSrc`, retaining the old GDI path as fallback. Verified result:

```text
wine365:kmt-svg-x11-xrender-final
  sha256:af792565f932f47b09f031c28c783ade97acb0061140e16231d330aa44d6c38a
/mnt/backup/wine365-reference/cache/
  wineprefix-word-test-x11-xrender-final-interact-20260713
artifacts/research/word-x11-xrender-final-interact-20260713/
```

The visible sequence contains a readable Word start page, successful Blank
document creation, a readable editor, and the exact entered text
`Wine 365 final XRender test` with a caret and five-word count. This does
not weaken the verified KMT/keyed-mutex semantics. It is still experimental and
needs X11 regression review, clipping/visual/alpha coverage, and an integrated
build of both architectures.

## Other compatibility work in this snapshot

The snapshot includes earlier Office/App-V/licensing work in the same dirty tree:

- SPPC licensing APIs, license/property handling, token/HMAC-related behaviour,
  and SPPC tests.
- App package-family lookup needed by Office imports.
- App-V support exports, including generic AVL table operations,
  `RtlIsNameInExpression`, `NtQueryDirectoryFileEx`, and hard-link name API
  compatibility stubs.
- Kernel/version, cryptographic provider, VC runtime export, MSXML/XPath/schema,
  and bundled libxml2 compatibility changes found during Office startup and
  licensing work.

These changes were developed together in the investigation tree and are preserved
as a single checkpoint commit.  They should be split by subsystem, reviewed
against native behaviour, and expanded with focused tests before any upstream
submission.

### Known review findings

A post-checkpoint source review identified concrete limitations that should remain
visible when this branch is used:

- The list-backed generic AVL table does not provide comparison-ordered
  enumeration, and restart-key handling can restart enumeration after exhaustion.
- `NtQueryDirectoryFileEx` only approximates some query flags;
  `SL_NO_CURSOR_UPDATE_QUERY` and `SL_INDEX_SPECIFIED` do not have native
  semantics.
- Several newly exported APIs lack complete public header declarations, and
  `IPackageManager6` is hand-declared instead of generated from IDL.
- SPPC authentication capture is process-global rather than context-specific,
  replacement challenges can retain prior authentication state, and several
  license/SLID results deliberately assume the targeted Word Grace installation.
- The SPPC `ActivePlugins` test and implementation currently disagree on plugin
  ordering and need native revalidation.
- The XML regular-expression `\\u` parser still accepts an isolated low surrogate.
- ProtectionPolicyManager `QueryInterface` needs normal COM `AddRef` ownership,
  and its successful event/policy stubs are not general implementations.
- The five-byte detour diagnostic reads the supplied address before the underlying
  API validates it and therefore must not remain in a production build.

These findings do not invalidate the recorded Office startup causality, but they
do mean the checkpoint must not be treated as generally correct Wine behaviour.

## Temporary diagnostics intentionally retained

This checkpoint intentionally retains diagnostics used to establish causality:

- five-byte x86-64 detour discovery in `dlls/kernelbase/memory.c`;
- first-chance Microsoft C++ exception stack capture in
  `dlls/ntdll/exception.c`;
- broad COM/GlobalOptions tracing in `dlls/combase/combase.c`.

These produce noisy logs and are not production changes.

## Test environment and safety

- Base: Wine 11.12 (`wine-11.12`, commit
  `996020f410e7a1aa2dd6b44cf740854ea524d31a`).
- Build/test environment: Docker with software rendering through llvmpipe.
- Tests use disposable reflinked copies of the Office prefix.  The preserved
  production prefix must not be modified.
- Prefix updates after image rebuilds use `wineboot -u`.
- Typical overrides:
  `WINEDLLOVERRIDES=mscoree,mshtml=` and, for Office,
  `vcruntime140,vcruntime140_1=n,b`.

The successful startup experiment used a disposable prefix under
`/mnt/backup/wine365-reference/cache/`.  Research traces and screenshots are kept
outside this Wine repository in the surrounding project's `artifacts/research/`
directory.

## Next work

1. Review and regression-test XRender offscreen composition, including clipping,
   non-default visuals, alpha formats, and XRender-unavailable fallback.
2. Build the integrated tree for both i386 and x86_64 and repeat the Word
   start-page/create/type smoke test in another disposable prefix.
3. Replace the local SVG ABI with proper declarations and add focused tests for
   NULL-stream creation, root/child identity, and ownership.
4. Implement the SVG attribute/path subset observed in Word and real
   `DrawSvgDocument` behavior needed for complete icon rendering.
5. Move EnterpriseData into the proper WinRT DLL/IDL architecture and add tests.
6. Reduce the activation-filter workaround to a tested compatibility rule and
   remove broad diagnostics.
7. Keep Excel and PowerPoint out of scope until the final integrated Word build
   and regression checks pass.
