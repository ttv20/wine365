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

The Word process and main top-level window remain alive after the optional sign-in
dialog is closed.  The UI is **not usable yet**: captures show only white
rectangular surfaces, not the Word start page.  Excel and PowerPoint have not been
investigated because Word's rendering path remains unresolved.

No Office binary predicate is patched in the successful test.  Earlier binary
patches were used only in disposable prefixes to prove causality.

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

## Rendering blocker and DXGI experiment

Word currently creates D3D11/DXGI surfaces but paints a white main window.  The
most useful outstanding observations are:

- Word makes roughly 130 calls to `IDXGIResource::GetSharedHandle`.
- Wine 11.12 and current Proton branches return `E_NOTIMPL` from this method.
- Native D3D11 tests expect a non-shared resource to return `S_OK` with a null
  handle.
- Word uses flip-sequential swap effect `0x3`, which wined3d logs as
  unimplemented.
- `Present1` parameters are partly ignored, and `DiscardView1` is a stub.

`dlls/dxgi/resource.c` currently contains a diagnostic experiment that always
returns `S_OK` and a null handle (with argument validation).  It has built, but was
not tested before this snapshot was committed.  It is not a complete shared
resource implementation and is incorrect for genuinely shared resources.  Before
retaining this behaviour, Wine must inspect or preserve the resource sharing flags
and return a real shared handle where required.

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

1. Test the null-shared-handle DXGI image on a fresh disposable prefix and record
   whether the white surface changes.
2. Record the queried resources' sharing flags and implement native-compatible
   conditional `GetSharedHandle` behaviour.
3. Investigate flip-sequential presentation and the remaining D3D11/DXGI stubs.
4. Move EnterpriseData into the proper WinRT DLL/IDL architecture and add tests.
5. Reduce the activation-filter workaround to a tested compatibility rule and
   remove broad diagnostics.
6. Rebuild and verify that Word can paint, create, and open a document before
   beginning Excel or PowerPoint work.
