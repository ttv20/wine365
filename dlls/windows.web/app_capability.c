/* WinRT Windows.Security.Authorization.AppCapabilityAccess implementation
 *
 * Copyright (C) 2026 Olivia Ryan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(web);

struct app_capability
{
    IAppCapability IAppCapability_iface;
    LONG ref;
    HSTRING name;
};

static inline struct app_capability *impl_from_IAppCapability( IAppCapability *iface )
{
    return CONTAINING_RECORD( iface, struct app_capability, IAppCapability_iface );
}

static HRESULT WINAPI app_capability_QueryInterface( IAppCapability *iface, REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) || IsEqualGUID( iid, &IID_IAppCapability ))
    {
        *out = iface;
        IAppCapability_AddRef( iface );
        return S_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI app_capability_AddRef( IAppCapability *iface )
{
    struct app_capability *impl = impl_from_IAppCapability( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI app_capability_Release( IAppCapability *iface )
{
    struct app_capability *impl = impl_from_IAppCapability( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    if (!ref)
    {
        WindowsDeleteString( impl->name );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI app_capability_GetIids( IAppCapability *iface, ULONG *count, IID **iids )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI app_capability_GetRuntimeClassName( IAppCapability *iface, HSTRING *name )
{
    return WindowsCreateString( RuntimeClass_Windows_Security_Authorization_AppCapabilityAccess_AppCapability,
                                wcslen( RuntimeClass_Windows_Security_Authorization_AppCapabilityAccess_AppCapability ), name );
}

static HRESULT WINAPI app_capability_GetTrustLevel( IAppCapability *iface, TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI app_capability_get_CapabilityName( IAppCapability *iface, HSTRING *value )
{
    struct app_capability *impl = impl_from_IAppCapability( iface );
    if (!value) return E_POINTER;
    return WindowsDuplicateString( impl->name, value );
}

static HRESULT WINAPI app_capability_get_User( IAppCapability *iface, __x_ABI_CWindows_CSystem_CIUser **value )
{
    if (!value) return E_POINTER;
    *value = NULL;
    return S_OK;
}

static HRESULT WINAPI app_capability_RequestAccessAsync( IAppCapability *iface,
        IAsyncOperation_AppCapabilityAccessStatus **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    if (operation) *operation = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI app_capability_CheckAccess( IAppCapability *iface, AppCapabilityAccessStatus *result )
{
    struct app_capability *impl = impl_from_IAppCapability( iface );
    if (!result) return E_POINTER;
    TRACE( "capability %s is not declared by desktop process.\n", debugstr_hstring( impl->name ) );
    *result = AppCapabilityAccessStatus_NotDeclaredByApp;
    return S_OK;
}

static HRESULT WINAPI app_capability_add_AccessChanged( IAppCapability *iface,
        ITypedEventHandler_AppCapability_AppCapabilityAccessChangedEventArgs *handler,
        EventRegistrationToken *token )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI app_capability_remove_AccessChanged( IAppCapability *iface, EventRegistrationToken token )
{
    return E_NOTIMPL;
}

static const IAppCapabilityVtbl app_capability_vtbl =
{
    app_capability_QueryInterface,
    app_capability_AddRef,
    app_capability_Release,
    app_capability_GetIids,
    app_capability_GetRuntimeClassName,
    app_capability_GetTrustLevel,
    app_capability_get_CapabilityName,
    app_capability_get_User,
    app_capability_RequestAccessAsync,
    app_capability_CheckAccess,
    app_capability_add_AccessChanged,
    app_capability_remove_AccessChanged,
};

struct app_capability_statics
{
    IActivationFactory IActivationFactory_iface;
    IAppCapabilityStatics IAppCapabilityStatics_iface;
    LONG ref;
};

static inline struct app_capability_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct app_capability_statics, IActivationFactory_iface );
}

static inline struct app_capability_statics *impl_from_IAppCapabilityStatics( IAppCapabilityStatics *iface )
{
    return CONTAINING_RECORD( iface, struct app_capability_statics, IAppCapabilityStatics_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct app_capability_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (!out) return E_POINTER;
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) || IsEqualGUID( iid, &IID_IActivationFactory ))
        *out = &impl->IActivationFactory_iface;
    else if (IsEqualGUID( iid, &IID_IAppCapabilityStatics ))
        *out = &impl->IAppCapabilityStatics_iface;
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }
    IInspectable_AddRef( *out );
    return S_OK;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    return InterlockedIncrement( &impl_from_IActivationFactory( iface )->ref );
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    return InterlockedDecrement( &impl_from_IActivationFactory( iface )->ref );
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *count, IID **iids )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *name )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *level )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    return E_NOTIMPL;
}

static const IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    factory_ActivateInstance,
};

static HRESULT WINAPI statics_QueryInterface( IAppCapabilityStatics *iface, REFIID iid, void **out )
{
    struct app_capability_statics *impl = impl_from_IAppCapabilityStatics( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG WINAPI statics_AddRef( IAppCapabilityStatics *iface )
{
    struct app_capability_statics *impl = impl_from_IAppCapabilityStatics( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG WINAPI statics_Release( IAppCapabilityStatics *iface )
{
    struct app_capability_statics *impl = impl_from_IAppCapabilityStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI statics_GetIids( IAppCapabilityStatics *iface, ULONG *count, IID **iids )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_GetRuntimeClassName( IAppCapabilityStatics *iface, HSTRING *name )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_GetTrustLevel( IAppCapabilityStatics *iface, TrustLevel *level )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_RequestAccessForCapabilitiesAsync( IAppCapabilityStatics *iface,
        IIterable_HSTRING *names, IAsyncOperation_IMapView_HSTRING_AppCapabilityAccessStatus **operation )
{
    FIXME( "iface %p, names %p, operation %p stub!\n", iface, names, operation );
    if (operation) *operation = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_RequestAccessForCapabilitiesForUserAsync( IAppCapabilityStatics *iface,
        __x_ABI_CWindows_CSystem_CIUser *user, IIterable_HSTRING *names,
        IAsyncOperation_IMapView_HSTRING_AppCapabilityAccessStatus **operation )
{
    FIXME( "iface %p, user %p, names %p, operation %p stub!\n", iface, user, names, operation );
    if (operation) *operation = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_Create( IAppCapabilityStatics *iface, HSTRING name, IAppCapability **result )
{
    struct app_capability *impl;
    HRESULT hr;

    if (!result) return E_POINTER;
    *result = NULL;
    if (!name) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IAppCapability_iface.lpVtbl = &app_capability_vtbl;
    impl->ref = 1;
    if (FAILED(hr = WindowsDuplicateString( name, &impl->name )))
    {
        free( impl );
        return hr;
    }
    TRACE( "created capability %s.\n", debugstr_hstring( name ) );
    *result = &impl->IAppCapability_iface;
    return S_OK;
}

static HRESULT WINAPI statics_CreateWithProcessIdForUser( IAppCapabilityStatics *iface,
        __x_ABI_CWindows_CSystem_CIUser *user, HSTRING name, UINT32 pid, IAppCapability **result )
{
    return statics_Create( iface, name, result );
}

static const IAppCapabilityStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    statics_RequestAccessForCapabilitiesAsync,
    statics_RequestAccessForCapabilitiesForUserAsync,
    statics_Create,
    statics_CreateWithProcessIdForUser,
};

static struct app_capability_statics app_capability_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    1,
};

IActivationFactory *app_capability_factory = &app_capability_statics.IActivationFactory_iface;
