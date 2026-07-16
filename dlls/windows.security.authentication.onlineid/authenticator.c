/* WinRT Windows.Security.Authentication.Onlineid Implementation
 *
 * Copyright (C) 2024 Mohamad Al-Jaf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(onlineid);

/* windows.security.authentication.web.core.idl is not available in Wine yet.
 * Keep these two ABI declarations local until the corresponding public WinRT
 * interfaces are added.  Office queries both interfaces before starting its
 * interactive account flow. */
static const GUID IID_IWebAuthenticationCoreManagerStatics =
    {0x6aca7c92, 0xa581, 0x4479, {0x9c, 0x10, 0x75, 0x2e, 0xff, 0x44, 0xfd, 0x34}};
static const GUID IID_IWebAuthenticationCoreManagerStatics4 =
    {0x54e633fe, 0x96e0, 0x41e8, {0x98, 0x32, 0x12, 0x98, 0x89, 0x7c, 0x2a, 0xaf}};
/* Desktop Office obtains this non-WinRT interop interface from the manager
 * activation factory when an interactive request needs an owner window. */
static const GUID IID_IWebAuthenticationCoreManagerInterop =
    {0xf4b8e804, 0x811e, 0x4436, {0xb6, 0x9c, 0x44, 0xcb, 0x67, 0xb7, 0x20, 0x84}};
static const GUID IID_IWebAccountProvider =
    {0x29dcc8c3, 0x7ab9, 0x4a7c, {0xa3, 0x36, 0xb9, 0x42, 0xf9, 0xdb, 0xf7, 0xc7}};
static const GUID IID_IWebAccountProvider2 =
    {0x4a01eb05, 0x4e42, 0x41d4, {0xb5, 0x18, 0xe0, 0x08, 0xa5, 0x16, 0x36, 0x14}};
static const GUID IID_IWebTokenRequest =
    {0xb77b4d68, 0xadcb, 0x4673, {0xb3, 0x64, 0x0c, 0xf7, 0xb3, 0x5c, 0xaf, 0x97}};
static const GUID IID_IWebTokenRequest3 =
    {0x5a755b51, 0x3bb1, 0x41a5, {0xa6, 0x3d, 0x90, 0xbc, 0x32, 0xc7, 0xdb, 0x9a}};
static const GUID IID_IWebTokenRequestFactory =
    {0x6cf2141c, 0x0ff0, 0x4c67, {0xb8, 0x4f, 0x99, 0xdd, 0xbe, 0x4a, 0x72, 0xc9}};

struct web_token_request_factory;
struct web_token_request_factory_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_token_request_factory *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_token_request_factory *);
    ULONG (WINAPI *Release)(struct web_token_request_factory *);
    HRESULT (WINAPI *GetIids)(struct web_token_request_factory *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_token_request_factory *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_token_request_factory *, TrustLevel *);
    HRESULT (WINAPI *Create)(struct web_token_request_factory *, IInspectable *, HSTRING, HSTRING, IInspectable **);
    HRESULT (WINAPI *CreateWithPromptType)(struct web_token_request_factory *, IInspectable *, HSTRING, HSTRING,
                                           INT32, IInspectable **);
    HRESULT (WINAPI *CreateWithProvider)(struct web_token_request_factory *, IInspectable *, IInspectable **);
    HRESULT (WINAPI *CreateWithScope)(struct web_token_request_factory *, IInspectable *, HSTRING, IInspectable **);
};

struct web_token_request_factory
{
    const struct web_token_request_factory_vtbl *lpVtbl;
};

/* These local ABI objects are intentionally small.  They cover the part of
 * Windows.Security.Credentials that Office consumes before constructing its
 * first WebTokenRequest.  The public Wine IDL does not expose WebAccountProvider
 * yet, so keep the declarations private to this implementation. */
struct web_account_provider;

struct web_account_provider_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_account_provider *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_account_provider *);
    ULONG (WINAPI *Release)(struct web_account_provider *);
    HRESULT (WINAPI *GetIids)(struct web_account_provider *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_account_provider *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_account_provider *, TrustLevel *);
    HRESULT (WINAPI *get_Id)(struct web_account_provider *, HSTRING *);
    HRESULT (WINAPI *get_DisplayName)(struct web_account_provider *, HSTRING *);
    HRESULT (WINAPI *get_IconUri)(struct web_account_provider *, IInspectable **);
};

struct web_account_provider2_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_account_provider *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_account_provider *);
    ULONG (WINAPI *Release)(struct web_account_provider *);
    HRESULT (WINAPI *GetIids)(struct web_account_provider *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_account_provider *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_account_provider *, TrustLevel *);
    HRESULT (WINAPI *get_DisplayPurpose)(struct web_account_provider *, HSTRING *);
    HRESULT (WINAPI *get_Authority)(struct web_account_provider *, HSTRING *);
};

struct web_account_provider
{
    const struct web_account_provider_vtbl *lpVtbl;
    const struct web_account_provider2_vtbl *provider2_vtbl;
    LONG ref;
    HSTRING id;
    HSTRING authority;
    HSTRING display_name;
    HSTRING display_purpose;
};

static inline struct web_account_provider *provider_from_provider2( struct web_account_provider *iface )
{
    return CONTAINING_RECORD( &iface->lpVtbl, struct web_account_provider, provider2_vtbl );
}

static HRESULT WINAPI provider_QueryInterface( struct web_account_provider *iface, REFIID iid, void **out )
{
    struct web_account_provider *impl = iface;

    if (!out) return E_POINTER;
    *out = NULL;
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) || IsEqualGUID( iid, &IID_IWebAccountProvider ))
        *out = impl;
    else if (IsEqualGUID( iid, &IID_IWebAccountProvider2 ))
        *out = &impl->provider2_vtbl;
    else
    {
        FIXME( "provider interface %s not implemented.\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }

    InterlockedIncrement( &impl->ref );
    return S_OK;
}

static HRESULT WINAPI provider2_QueryInterface( struct web_account_provider *iface, REFIID iid, void **out )
{
    return provider_QueryInterface( provider_from_provider2( iface ), iid, out );
}

static ULONG WINAPI provider_AddRef( struct web_account_provider *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI provider_Release( struct web_account_provider *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        WindowsDeleteString( iface->id );
        WindowsDeleteString( iface->authority );
        WindowsDeleteString( iface->display_name );
        WindowsDeleteString( iface->display_purpose );
        free( iface );
    }
    return ref;
}

static ULONG WINAPI provider2_AddRef( struct web_account_provider *iface )
{
    return provider_AddRef( provider_from_provider2( iface ) );
}

static ULONG WINAPI provider2_Release( struct web_account_provider *iface )
{
    return provider_Release( provider_from_provider2( iface ) );
}

static HRESULT provider_get_iids( ULONG *iid_count, IID **iids )
{
    if (!iid_count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( 2 * sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IWebAccountProvider;
    (*iids)[1] = IID_IWebAccountProvider2;
    *iid_count = 2;
    return S_OK;
}

static HRESULT WINAPI provider_GetIids( struct web_account_provider *iface, ULONG *iid_count, IID **iids )
{
    return provider_get_iids( iid_count, iids );
}

static HRESULT WINAPI provider2_GetIids( struct web_account_provider *iface, ULONG *iid_count, IID **iids )
{
    return provider_get_iids( iid_count, iids );
}

static HRESULT provider_get_runtime_class_name( HSTRING *class_name )
{
    static const WCHAR name[] = L"Windows.Security.Credentials.WebAccountProvider";
    if (!class_name) return E_POINTER;
    return WindowsCreateString( name, ARRAY_SIZE(name) - 1, class_name );
}

static HRESULT WINAPI provider_GetRuntimeClassName( struct web_account_provider *iface, HSTRING *class_name )
{
    return provider_get_runtime_class_name( class_name );
}

static HRESULT WINAPI provider2_GetRuntimeClassName( struct web_account_provider *iface, HSTRING *class_name )
{
    return provider_get_runtime_class_name( class_name );
}

static HRESULT provider_get_trust_level( TrustLevel *trust_level )
{
    if (!trust_level) return E_POINTER;
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI provider_GetTrustLevel( struct web_account_provider *iface, TrustLevel *trust_level )
{
    return provider_get_trust_level( trust_level );
}

static HRESULT WINAPI provider2_GetTrustLevel( struct web_account_provider *iface, TrustLevel *trust_level )
{
    return provider_get_trust_level( trust_level );
}

static HRESULT WINAPI provider_get_Id( struct web_account_provider *iface, HSTRING *value )
{
    if (!value) return E_POINTER;
    return WindowsDuplicateString( iface->id, value );
}

static HRESULT WINAPI provider_get_DisplayName( struct web_account_provider *iface, HSTRING *value )
{
    if (!value) return E_POINTER;
    return WindowsDuplicateString( iface->display_name, value );
}

static HRESULT WINAPI provider_get_IconUri( struct web_account_provider *iface, IInspectable **value )
{
    if (!value) return E_POINTER;
    *value = NULL;
    return S_OK;
}

static HRESULT WINAPI provider2_get_DisplayPurpose( struct web_account_provider *iface, HSTRING *value )
{
    struct web_account_provider *impl = provider_from_provider2( iface );
    if (!value) return E_POINTER;
    return WindowsDuplicateString( impl->display_purpose, value );
}

static HRESULT WINAPI provider2_get_Authority( struct web_account_provider *iface, HSTRING *value )
{
    struct web_account_provider *impl = provider_from_provider2( iface );
    if (!value) return E_POINTER;
    return WindowsDuplicateString( impl->authority, value );
}

static const struct web_account_provider_vtbl provider_vtbl =
{
    provider_QueryInterface, provider_AddRef, provider_Release,
    provider_GetIids, provider_GetRuntimeClassName, provider_GetTrustLevel,
    provider_get_Id, provider_get_DisplayName, provider_get_IconUri,
};

static const struct web_account_provider2_vtbl provider2_vtbl =
{
    provider2_QueryInterface, provider2_AddRef, provider2_Release,
    provider2_GetIids, provider2_GetRuntimeClassName, provider2_GetTrustLevel,
    provider2_get_DisplayPurpose, provider2_get_Authority,
};

static HRESULT web_account_provider_create( HSTRING id, HSTRING authority, IInspectable **out )
{
    static const WCHAR display_name[] = L"Microsoft";
    struct web_account_provider *impl;
    HRESULT hr;

    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &provider_vtbl;
    impl->provider2_vtbl = &provider2_vtbl;
    impl->ref = 1;
    if (FAILED(hr = WindowsDuplicateString( id, &impl->id )) ||
        FAILED(hr = WindowsDuplicateString( authority, &impl->authority )) ||
        FAILED(hr = WindowsCreateString( display_name, ARRAY_SIZE(display_name) - 1, &impl->display_name )) ||
        FAILED(hr = WindowsCreateString( NULL, 0, &impl->display_purpose )))
    {
        provider_Release( impl );
        return hr;
    }
    *out = (IInspectable *)impl;
    return S_OK;
}

/* Office enumerates WebProviderError.Properties.  Returning the IMap vtable
 * for the generated IIterable<IKeyValuePair<HSTRING,HSTRING>> IID corrupts
 * the IIterable::First call and raises E_POINTER. */

struct string_map;
struct string_map_view;
struct string_map_iterator;
struct string_key_value_pair;

struct string_map_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct string_map *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct string_map *);
    ULONG (WINAPI *Release)(struct string_map *);
    HRESULT (WINAPI *GetIids)(struct string_map *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct string_map *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct string_map *, TrustLevel *);
    HRESULT (WINAPI *Lookup)(struct string_map *, HSTRING, HSTRING *);
    HRESULT (WINAPI *get_Size)(struct string_map *, UINT32 *);
    HRESULT (WINAPI *HasKey)(struct string_map *, HSTRING, boolean *);
    HRESULT (WINAPI *GetView)(struct string_map *, struct string_map_view **);
    HRESULT (WINAPI *Insert)(struct string_map *, HSTRING, HSTRING, boolean *);
    HRESULT (WINAPI *Remove)(struct string_map *, HSTRING);
    HRESULT (WINAPI *Clear)(struct string_map *);
};

struct string_iterable_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct string_map *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct string_map *);
    ULONG (WINAPI *Release)(struct string_map *);
    HRESULT (WINAPI *GetIids)(struct string_map *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct string_map *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct string_map *, TrustLevel *);
    HRESULT (WINAPI *First)(struct string_map *, struct string_map_iterator **);
};

struct string_map_view_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct string_map_view *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct string_map_view *);
    ULONG (WINAPI *Release)(struct string_map_view *);
    HRESULT (WINAPI *GetIids)(struct string_map_view *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct string_map_view *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct string_map_view *, TrustLevel *);
    HRESULT (WINAPI *Lookup)(struct string_map_view *, HSTRING, HSTRING *);
    HRESULT (WINAPI *get_Size)(struct string_map_view *, UINT32 *);
    HRESULT (WINAPI *HasKey)(struct string_map_view *, HSTRING, boolean *);
    HRESULT (WINAPI *Split)(struct string_map_view *, struct string_map_view **, struct string_map_view **);
};

struct string_view_iterable_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct string_map_view *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct string_map_view *);
    ULONG (WINAPI *Release)(struct string_map_view *);
    HRESULT (WINAPI *GetIids)(struct string_map_view *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct string_map_view *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct string_map_view *, TrustLevel *);
    HRESULT (WINAPI *First)(struct string_map_view *, struct string_map_iterator **);
};

struct string_map_iterator_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct string_map_iterator *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct string_map_iterator *);
    ULONG (WINAPI *Release)(struct string_map_iterator *);
    HRESULT (WINAPI *GetIids)(struct string_map_iterator *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct string_map_iterator *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct string_map_iterator *, TrustLevel *);
    HRESULT (WINAPI *get_Current)(struct string_map_iterator *, struct string_key_value_pair **);
    HRESULT (WINAPI *get_HasCurrent)(struct string_map_iterator *, boolean *);
    HRESULT (WINAPI *MoveNext)(struct string_map_iterator *, boolean *);
    HRESULT (WINAPI *GetMany)(struct string_map_iterator *, UINT32, struct string_key_value_pair **, UINT32 *);
};

struct string_key_value_pair_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct string_key_value_pair *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct string_key_value_pair *);
    ULONG (WINAPI *Release)(struct string_key_value_pair *);
    HRESULT (WINAPI *GetIids)(struct string_key_value_pair *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct string_key_value_pair *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct string_key_value_pair *, TrustLevel *);
    HRESULT (WINAPI *get_Key)(struct string_key_value_pair *, HSTRING *);
    HRESULT (WINAPI *get_Value)(struct string_key_value_pair *, HSTRING *);
};

struct string_map_entry
{
    HSTRING key;
    HSTRING value;
};

struct string_map
{
    const struct string_map_vtbl *lpVtbl;
    const struct string_iterable_vtbl *iterable_vtbl;
    LONG ref;
    UINT32 size;
    struct string_map_entry entries[16];
};

struct string_map_view
{
    const struct string_map_view_vtbl *lpVtbl;
    const struct string_view_iterable_vtbl *iterable_vtbl;
    LONG ref;
    UINT32 size;
    struct string_map_entry entries[16];
};

struct string_map_iterator
{
    const struct string_map_iterator_vtbl *lpVtbl;
    LONG ref;
    UINT32 size;
    UINT32 index;
    struct string_map_entry entries[16];
};

struct string_key_value_pair
{
    const struct string_key_value_pair_vtbl *lpVtbl;
    LONG ref;
    HSTRING key;
    HSTRING value;
};

static int string_entries_find( const struct string_map_entry *entries, UINT32 size, HSTRING key )
{
    const WCHAR *needle = WindowsGetStringRawBuffer( key, NULL );
    UINT32 i;
    for (i = 0; i < size; ++i)
        if (!wcscmp( WindowsGetStringRawBuffer( entries[i].key, NULL ), needle )) return i;
    return -1;
}

static void string_entries_clear( struct string_map_entry *entries, UINT32 size )
{
    UINT32 i;
    for (i = 0; i < size; ++i)
    {
        WindowsDeleteString( entries[i].key );
        WindowsDeleteString( entries[i].value );
        entries[i].key = entries[i].value = NULL;
    }
}

static HRESULT string_entries_copy( struct string_map_entry *dst, const struct string_map_entry *src, UINT32 size )
{
    UINT32 i;
    HRESULT hr;

    for (i = 0; i < size; ++i)
    {
        if (FAILED(hr = WindowsDuplicateString( src[i].key, &dst[i].key )) ||
            FAILED(hr = WindowsDuplicateString( src[i].value, &dst[i].value )))
        {
            string_entries_clear( dst, i + 1 );
            return hr;
        }
    }
    return S_OK;
}

static HRESULT string_collection_get_iids( const GUID *first, const GUID *second, ULONG *count, IID **iids )
{
    ULONG iid_count = second ? 2 : 1;

    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( iid_count * sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = *first;
    if (second) (*iids)[1] = *second;
    *count = iid_count;
    return S_OK;
}

static HRESULT string_collection_runtime_name( const WCHAR *class_name, HSTRING *name )
{
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, wcslen( class_name ), name );
}

static HRESULT string_collection_trust_level( TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI string_pair_QueryInterface( struct string_key_value_pair *iface, REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!IsEqualGUID( iid, &IID_IUnknown ) && !IsEqualGUID( iid, &IID_IInspectable ) &&
        !IsEqualGUID( iid, &IID_IAgileObject ) && !IsEqualGUID( iid, &IID_IKeyValuePair_HSTRING_HSTRING ))
        return E_NOINTERFACE;
    *out = iface;
    InterlockedIncrement( &iface->ref );
    return S_OK;
}

static ULONG WINAPI string_pair_AddRef( struct string_key_value_pair *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI string_pair_Release( struct string_key_value_pair *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        WindowsDeleteString( iface->key );
        WindowsDeleteString( iface->value );
        free( iface );
    }
    return ref;
}

static HRESULT WINAPI string_pair_GetIids( struct string_key_value_pair *iface, ULONG *count, IID **iids )
{
    return string_collection_get_iids( &IID_IKeyValuePair_HSTRING_HSTRING, NULL, count, iids );
}

static HRESULT WINAPI string_pair_GetRuntimeClassName( struct string_key_value_pair *iface, HSTRING *name )
{
    return string_collection_runtime_name( L"Windows.Foundation.Collections.IKeyValuePair`2<String,String>", name );
}

static HRESULT WINAPI string_pair_GetTrustLevel( struct string_key_value_pair *iface, TrustLevel *level )
{
    return string_collection_trust_level( level );
}

static HRESULT WINAPI string_pair_get_Key( struct string_key_value_pair *iface, HSTRING *value )
{
    if (!value) return E_POINTER;
    return WindowsDuplicateString( iface->key, value );
}

static HRESULT WINAPI string_pair_get_Value( struct string_key_value_pair *iface, HSTRING *value )
{
    if (!value) return E_POINTER;
    return WindowsDuplicateString( iface->value, value );
}

static const struct string_key_value_pair_vtbl string_pair_vtbl =
{
    string_pair_QueryInterface, string_pair_AddRef, string_pair_Release,
    string_pair_GetIids, string_pair_GetRuntimeClassName, string_pair_GetTrustLevel,
    string_pair_get_Key, string_pair_get_Value,
};

static HRESULT string_pair_create( const struct string_map_entry *entry, struct string_key_value_pair **out )
{
    struct string_key_value_pair *impl;
    HRESULT hr;

    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &string_pair_vtbl;
    impl->ref = 1;
    if (FAILED(hr = WindowsDuplicateString( entry->key, &impl->key )) ||
        FAILED(hr = WindowsDuplicateString( entry->value, &impl->value )))
    {
        string_pair_Release( impl );
        return hr;
    }
    *out = impl;
    return S_OK;
}

static HRESULT WINAPI string_iterator_QueryInterface( struct string_map_iterator *iface, REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!IsEqualGUID( iid, &IID_IUnknown ) && !IsEqualGUID( iid, &IID_IInspectable ) &&
        !IsEqualGUID( iid, &IID_IAgileObject ) &&
        !IsEqualGUID( iid, &IID_IIterator_IKeyValuePair_HSTRING_HSTRING ))
        return E_NOINTERFACE;
    *out = iface;
    InterlockedIncrement( &iface->ref );
    return S_OK;
}

static ULONG WINAPI string_iterator_AddRef( struct string_map_iterator *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI string_iterator_Release( struct string_map_iterator *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        string_entries_clear( iface->entries, iface->size );
        free( iface );
    }
    return ref;
}

static HRESULT WINAPI string_iterator_GetIids( struct string_map_iterator *iface, ULONG *count, IID **iids )
{
    return string_collection_get_iids( &IID_IIterator_IKeyValuePair_HSTRING_HSTRING, NULL, count, iids );
}

static HRESULT WINAPI string_iterator_GetRuntimeClassName( struct string_map_iterator *iface, HSTRING *name )
{
    return string_collection_runtime_name( L"Windows.Foundation.Collections.IIterator`1<IKeyValuePair`2<String,String>>", name );
}

static HRESULT WINAPI string_iterator_GetTrustLevel( struct string_map_iterator *iface, TrustLevel *level )
{
    return string_collection_trust_level( level );
}

static HRESULT WINAPI string_iterator_get_Current( struct string_map_iterator *iface,
                                                    struct string_key_value_pair **value )
{
    if (!value) return E_POINTER;
    *value = NULL;
    if (iface->index >= iface->size) return E_BOUNDS;
    return string_pair_create( &iface->entries[iface->index], value );
}

static HRESULT WINAPI string_iterator_get_HasCurrent( struct string_map_iterator *iface, boolean *value )
{
    if (!value) return E_POINTER;
    *value = iface->index < iface->size;
    TRACE( "iterator %p has current %u at %u/%u.\n", iface, *value, iface->index, iface->size );
    return S_OK;
}

static HRESULT WINAPI string_iterator_MoveNext( struct string_map_iterator *iface, boolean *value )
{
    if (!value) return E_POINTER;
    if (iface->index < iface->size) ++iface->index;
    *value = iface->index < iface->size;
    TRACE( "iterator %p moved to %u/%u, has current %u.\n", iface, iface->index, iface->size, *value );
    return S_OK;
}

static HRESULT WINAPI string_iterator_GetMany( struct string_map_iterator *iface, UINT32 capacity,
                                                struct string_key_value_pair **items, UINT32 *count )
{
    UINT32 i, available;
    HRESULT hr;

    if (!count || (capacity && !items)) return E_POINTER;
    *count = 0;
    available = min( capacity, iface->size - iface->index );
    for (i = 0; i < available; ++i)
    {
        if (FAILED(hr = string_pair_create( &iface->entries[iface->index + i], &items[i] )))
        {
            while (i) string_pair_Release( items[--i] );
            return hr;
        }
    }
    iface->index += available;
    *count = available;
    return S_OK;
}

static const struct string_map_iterator_vtbl string_iterator_vtbl =
{
    string_iterator_QueryInterface, string_iterator_AddRef, string_iterator_Release,
    string_iterator_GetIids, string_iterator_GetRuntimeClassName, string_iterator_GetTrustLevel,
    string_iterator_get_Current, string_iterator_get_HasCurrent, string_iterator_MoveNext,
    string_iterator_GetMany,
};

static HRESULT string_iterator_create( const struct string_map_entry *entries, UINT32 size,
                                       struct string_map_iterator **out )
{
    struct string_map_iterator *impl;
    HRESULT hr;

    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &string_iterator_vtbl;
    impl->ref = 1;
    if (FAILED(hr = string_entries_copy( impl->entries, entries, size )))
    {
        free( impl );
        return hr;
    }
    impl->size = size;
    *out = impl;
    TRACE( "created iterator %p with %u entries.\n", impl, size );
    return S_OK;
}

static inline struct string_map *string_map_from_iterable( struct string_map *iface )
{
    return CONTAINING_RECORD( &iface->lpVtbl, struct string_map, iterable_vtbl );
}

static HRESULT WINAPI string_map_QueryInterface( struct string_map *iface, REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) || IsEqualGUID( iid, &IID_IMap_HSTRING_HSTRING ))
        *out = iface;
    else if (IsEqualGUID( iid, &IID_IIterable_IKeyValuePair_HSTRING_HSTRING ))
        *out = &iface->iterable_vtbl;
    else
    {
        TRACE( "map interface %s is not implemented.\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }
    InterlockedIncrement( &iface->ref );
    return S_OK;
}

static HRESULT WINAPI string_map_iterable_QueryInterface( struct string_map *iface, REFIID iid, void **out )
{
    return string_map_QueryInterface( string_map_from_iterable( iface ), iid, out );
}

static ULONG WINAPI string_map_AddRef( struct string_map *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI string_map_iterable_AddRef( struct string_map *iface )
{
    return string_map_AddRef( string_map_from_iterable( iface ) );
}

static HRESULT WINAPI string_map_Clear( struct string_map *iface )
{
    string_entries_clear( iface->entries, iface->size );
    iface->size = 0;
    return S_OK;
}

static ULONG WINAPI string_map_Release( struct string_map *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        string_map_Clear( iface );
        free( iface );
    }
    return ref;
}

static ULONG WINAPI string_map_iterable_Release( struct string_map *iface )
{
    return string_map_Release( string_map_from_iterable( iface ) );
}

static HRESULT WINAPI string_map_GetIids( struct string_map *iface, ULONG *count, IID **iids )
{
    return string_collection_get_iids( &IID_IMap_HSTRING_HSTRING,
                                       &IID_IIterable_IKeyValuePair_HSTRING_HSTRING, count, iids );
}

static HRESULT WINAPI string_map_iterable_GetIids( struct string_map *iface, ULONG *count, IID **iids )
{
    return string_map_GetIids( string_map_from_iterable( iface ), count, iids );
}

static HRESULT WINAPI string_map_GetRuntimeClassName( struct string_map *iface, HSTRING *name )
{
    return string_collection_runtime_name( L"Windows.Foundation.Collections.IMap`2<String,String>", name );
}

static HRESULT WINAPI string_map_iterable_GetRuntimeClassName( struct string_map *iface, HSTRING *name )
{
    return string_map_GetRuntimeClassName( string_map_from_iterable( iface ), name );
}

static HRESULT WINAPI string_map_GetTrustLevel( struct string_map *iface, TrustLevel *level )
{
    return string_collection_trust_level( level );
}

static HRESULT WINAPI string_map_iterable_GetTrustLevel( struct string_map *iface, TrustLevel *level )
{
    return string_map_GetTrustLevel( string_map_from_iterable( iface ), level );
}

static HRESULT WINAPI string_map_Lookup( struct string_map *iface, HSTRING key, HSTRING *value )
{
    int index;
    if (!value) return E_POINTER;
    *value = NULL;
    if ((index = string_entries_find( iface->entries, iface->size, key )) < 0) return E_BOUNDS;
    return WindowsDuplicateString( iface->entries[index].value, value );
}

static HRESULT WINAPI string_map_get_Size( struct string_map *iface, UINT32 *size )
{
    if (!size) return E_POINTER;
    *size = iface->size;
    return S_OK;
}

static HRESULT WINAPI string_map_HasKey( struct string_map *iface, HSTRING key, boolean *found )
{
    if (!found) return E_POINTER;
    *found = string_entries_find( iface->entries, iface->size, key ) >= 0;
    return S_OK;
}

static HRESULT string_map_view_create( const struct string_map_entry *, UINT32, struct string_map_view ** );

static HRESULT WINAPI string_map_GetView( struct string_map *iface, struct string_map_view **view )
{
    return string_map_view_create( iface->entries, iface->size, view );
}

static HRESULT WINAPI string_map_Insert( struct string_map *iface, HSTRING key, HSTRING value, boolean *replaced )
{
    HSTRING key_copy = NULL, value_copy = NULL;
    int index = string_entries_find( iface->entries, iface->size, key );
    HRESULT hr;

    if (!replaced) return E_POINTER;
    *replaced = index >= 0;
    if (FAILED(hr = WindowsDuplicateString( value, &value_copy ))) return hr;
    if (index >= 0)
    {
        WindowsDeleteString( iface->entries[index].value );
        iface->entries[index].value = value_copy;
        TRACE( "replaced property %s=%s.\n", debugstr_hstring( key ), debugstr_hstring( value ) );
        return S_OK;
    }
    if (iface->size == ARRAY_SIZE(iface->entries))
    {
        WindowsDeleteString( value_copy );
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr = WindowsDuplicateString( key, &key_copy )))
    {
        WindowsDeleteString( value_copy );
        return hr;
    }
    iface->entries[iface->size].key = key_copy;
    iface->entries[iface->size].value = value_copy;
    ++iface->size;
    TRACE( "inserted property %s=%s.\n", debugstr_hstring( key ), debugstr_hstring( value ) );
    return S_OK;
}

static HRESULT WINAPI string_map_Remove( struct string_map *iface, HSTRING key )
{
    int index = string_entries_find( iface->entries, iface->size, key );
    if (index < 0) return E_BOUNDS;
    WindowsDeleteString( iface->entries[index].key );
    WindowsDeleteString( iface->entries[index].value );
    memmove( &iface->entries[index], &iface->entries[index + 1],
             (iface->size - index - 1) * sizeof(iface->entries[0]) );
    --iface->size;
    iface->entries[iface->size].key = iface->entries[iface->size].value = NULL;
    return S_OK;
}

static HRESULT WINAPI string_map_iterable_First( struct string_map *iface, struct string_map_iterator **iterator )
{
    struct string_map *impl = string_map_from_iterable( iface );
    TRACE( "map %p First, iterator %p.\n", impl, iterator );
    return string_iterator_create( impl->entries, impl->size, iterator );
}

static const struct string_map_vtbl string_map_vtbl =
{
    string_map_QueryInterface, string_map_AddRef, string_map_Release,
    string_map_GetIids, string_map_GetRuntimeClassName, string_map_GetTrustLevel,
    string_map_Lookup, string_map_get_Size, string_map_HasKey, string_map_GetView,
    string_map_Insert, string_map_Remove, string_map_Clear,
};

static const struct string_iterable_vtbl string_map_iterable_vtbl =
{
    string_map_iterable_QueryInterface, string_map_iterable_AddRef, string_map_iterable_Release,
    string_map_iterable_GetIids, string_map_iterable_GetRuntimeClassName,
    string_map_iterable_GetTrustLevel, string_map_iterable_First,
};

static inline struct string_map_view *string_map_view_from_iterable( struct string_map_view *iface )
{
    return CONTAINING_RECORD( &iface->lpVtbl, struct string_map_view, iterable_vtbl );
}

static HRESULT WINAPI string_map_view_QueryInterface( struct string_map_view *iface, REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) || IsEqualGUID( iid, &IID_IMapView_HSTRING_HSTRING ))
        *out = iface;
    else if (IsEqualGUID( iid, &IID_IIterable_IKeyValuePair_HSTRING_HSTRING ))
        *out = &iface->iterable_vtbl;
    else
        return E_NOINTERFACE;
    InterlockedIncrement( &iface->ref );
    return S_OK;
}

static HRESULT WINAPI string_map_view_iterable_QueryInterface( struct string_map_view *iface, REFIID iid, void **out )
{
    return string_map_view_QueryInterface( string_map_view_from_iterable( iface ), iid, out );
}

static ULONG WINAPI string_map_view_AddRef( struct string_map_view *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI string_map_view_iterable_AddRef( struct string_map_view *iface )
{
    return string_map_view_AddRef( string_map_view_from_iterable( iface ) );
}

static ULONG WINAPI string_map_view_Release( struct string_map_view *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        string_entries_clear( iface->entries, iface->size );
        free( iface );
    }
    return ref;
}

static ULONG WINAPI string_map_view_iterable_Release( struct string_map_view *iface )
{
    return string_map_view_Release( string_map_view_from_iterable( iface ) );
}

static HRESULT WINAPI string_map_view_GetIids( struct string_map_view *iface, ULONG *count, IID **iids )
{
    return string_collection_get_iids( &IID_IMapView_HSTRING_HSTRING,
                                       &IID_IIterable_IKeyValuePair_HSTRING_HSTRING, count, iids );
}

static HRESULT WINAPI string_map_view_iterable_GetIids( struct string_map_view *iface, ULONG *count, IID **iids )
{
    return string_map_view_GetIids( string_map_view_from_iterable( iface ), count, iids );
}

static HRESULT WINAPI string_map_view_GetRuntimeClassName( struct string_map_view *iface, HSTRING *name )
{
    return string_collection_runtime_name( L"Windows.Foundation.Collections.IMapView`2<String,String>", name );
}

static HRESULT WINAPI string_map_view_iterable_GetRuntimeClassName( struct string_map_view *iface, HSTRING *name )
{
    return string_map_view_GetRuntimeClassName( string_map_view_from_iterable( iface ), name );
}

static HRESULT WINAPI string_map_view_GetTrustLevel( struct string_map_view *iface, TrustLevel *level )
{
    return string_collection_trust_level( level );
}

static HRESULT WINAPI string_map_view_iterable_GetTrustLevel( struct string_map_view *iface, TrustLevel *level )
{
    return string_map_view_GetTrustLevel( string_map_view_from_iterable( iface ), level );
}

static HRESULT WINAPI string_map_view_Lookup( struct string_map_view *iface, HSTRING key, HSTRING *value )
{
    int index;
    if (!value) return E_POINTER;
    *value = NULL;
    if ((index = string_entries_find( iface->entries, iface->size, key )) < 0) return E_BOUNDS;
    return WindowsDuplicateString( iface->entries[index].value, value );
}

static HRESULT WINAPI string_map_view_get_Size( struct string_map_view *iface, UINT32 *size )
{
    if (!size) return E_POINTER;
    *size = iface->size;
    return S_OK;
}

static HRESULT WINAPI string_map_view_HasKey( struct string_map_view *iface, HSTRING key, boolean *found )
{
    if (!found) return E_POINTER;
    *found = string_entries_find( iface->entries, iface->size, key ) >= 0;
    return S_OK;
}

static HRESULT WINAPI string_map_view_Split( struct string_map_view *iface, struct string_map_view **first,
                                             struct string_map_view **second )
{
    UINT32 midpoint;
    HRESULT hr;

    if (!first || !second) return E_POINTER;
    *first = *second = NULL;
    if (iface->size < 2) return S_OK;
    midpoint = iface->size / 2;
    if (FAILED(hr = string_map_view_create( iface->entries, midpoint, first ))) return hr;
    if (FAILED(hr = string_map_view_create( iface->entries + midpoint, iface->size - midpoint, second )))
    {
        string_map_view_Release( *first );
        *first = NULL;
    }
    return hr;
}

static HRESULT WINAPI string_map_view_iterable_First( struct string_map_view *iface,
                                                       struct string_map_iterator **iterator )
{
    struct string_map_view *impl = string_map_view_from_iterable( iface );
    TRACE( "map view %p First, iterator %p.\n", impl, iterator );
    return string_iterator_create( impl->entries, impl->size, iterator );
}

static const struct string_map_view_vtbl string_map_view_vtbl =
{
    string_map_view_QueryInterface, string_map_view_AddRef, string_map_view_Release,
    string_map_view_GetIids, string_map_view_GetRuntimeClassName, string_map_view_GetTrustLevel,
    string_map_view_Lookup, string_map_view_get_Size, string_map_view_HasKey, string_map_view_Split,
};

static const struct string_view_iterable_vtbl string_map_view_iterable_vtbl =
{
    string_map_view_iterable_QueryInterface, string_map_view_iterable_AddRef,
    string_map_view_iterable_Release, string_map_view_iterable_GetIids,
    string_map_view_iterable_GetRuntimeClassName, string_map_view_iterable_GetTrustLevel,
    string_map_view_iterable_First,
};

static HRESULT string_map_view_create( const struct string_map_entry *entries, UINT32 size,
                                       struct string_map_view **out )
{
    struct string_map_view *impl;
    HRESULT hr;

    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &string_map_view_vtbl;
    impl->iterable_vtbl = &string_map_view_iterable_vtbl;
    impl->ref = 1;
    if (FAILED(hr = string_entries_copy( impl->entries, entries, size )))
    {
        free( impl );
        return hr;
    }
    impl->size = size;
    *out = impl;
    return S_OK;
}

static HRESULT string_map_create( IInspectable **out )
{
    struct string_map *impl;
    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &string_map_vtbl;
    impl->iterable_vtbl = &string_map_iterable_vtbl;
    impl->ref = 1;
    *out = (IInspectable *)impl;
    return S_OK;
}

struct web_token_request;
struct web_token_request_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_token_request *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_token_request *);
    ULONG (WINAPI *Release)(struct web_token_request *);
    HRESULT (WINAPI *GetIids)(struct web_token_request *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_token_request *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_token_request *, TrustLevel *);
    HRESULT (WINAPI *get_WebAccountProvider)(struct web_token_request *, IInspectable **);
    HRESULT (WINAPI *get_Scope)(struct web_token_request *, HSTRING *);
    HRESULT (WINAPI *get_ClientId)(struct web_token_request *, HSTRING *);
    HRESULT (WINAPI *get_PromptType)(struct web_token_request *, INT32 *);
    HRESULT (WINAPI *get_Properties)(struct web_token_request *, IInspectable **);
};

struct web_token_request3_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_token_request *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_token_request *);
    ULONG (WINAPI *Release)(struct web_token_request *);
    HRESULT (WINAPI *GetIids)(struct web_token_request *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_token_request *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_token_request *, TrustLevel *);
    HRESULT (WINAPI *get_CorrelationId)(struct web_token_request *, HSTRING *);
    HRESULT (WINAPI *put_CorrelationId)(struct web_token_request *, HSTRING);
};

struct web_token_request
{
    const struct web_token_request_vtbl *lpVtbl;
    const struct web_token_request3_vtbl *request3_vtbl;
    LONG ref;
    IInspectable *provider;
    IInspectable *properties;
    HSTRING scope;
    HSTRING client_id;
    HSTRING correlation_id;
    INT32 prompt_type;
};

static inline struct web_token_request *token_request_from_request3( struct web_token_request *iface )
{
    return CONTAINING_RECORD( &iface->lpVtbl, struct web_token_request, request3_vtbl );
}

static HRESULT WINAPI token_request_QueryInterface( struct web_token_request *iface, REFIID iid, void **out )
{
    struct web_token_request *impl = iface;

    if (!out) return E_POINTER;
    *out = NULL;
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) || IsEqualGUID( iid, &IID_IWebTokenRequest ))
        *out = impl;
    else if (IsEqualGUID( iid, &IID_IWebTokenRequest3 ))
        *out = &impl->request3_vtbl;
    else
    {
        FIXME( "request interface %s not implemented.\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }
    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), *out );
    InterlockedIncrement( &impl->ref );
    return S_OK;
}

static HRESULT WINAPI token_request3_QueryInterface( struct web_token_request *iface, REFIID iid, void **out )
{
    return token_request_QueryInterface( token_request_from_request3( iface ), iid, out );
}

static ULONG WINAPI token_request_AddRef( struct web_token_request *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI token_request3_AddRef( struct web_token_request *iface )
{
    return token_request_AddRef( token_request_from_request3( iface ) );
}

static ULONG WINAPI token_request_Release( struct web_token_request *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        if (iface->provider) IInspectable_Release( iface->provider );
        if (iface->properties) IInspectable_Release( iface->properties );
        WindowsDeleteString( iface->scope );
        WindowsDeleteString( iface->client_id );
        WindowsDeleteString( iface->correlation_id );
        free( iface );
    }
    return ref;
}

static ULONG WINAPI token_request3_Release( struct web_token_request *iface )
{
    return token_request_Release( token_request_from_request3( iface ) );
}

static HRESULT WINAPI token_request_GetIids( struct web_token_request *iface, ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( 2 * sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IWebTokenRequest;
    (*iids)[1] = IID_IWebTokenRequest3;
    *count = 2;
    return S_OK;
}

static HRESULT WINAPI token_request3_GetIids( struct web_token_request *iface, ULONG *count, IID **iids )
{
    return token_request_GetIids( token_request_from_request3( iface ), count, iids );
}

static HRESULT WINAPI token_request_GetRuntimeClassName( struct web_token_request *iface, HSTRING *name )
{
    static const WCHAR class_name[] = L"Windows.Security.Authentication.Web.Core.WebTokenRequest";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI token_request3_GetRuntimeClassName( struct web_token_request *iface, HSTRING *name )
{
    return token_request_GetRuntimeClassName( token_request_from_request3( iface ), name );
}

static HRESULT WINAPI token_request_GetTrustLevel( struct web_token_request *iface, TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI token_request3_GetTrustLevel( struct web_token_request *iface, TrustLevel *level )
{
    return token_request_GetTrustLevel( token_request_from_request3( iface ), level );
}

static HRESULT WINAPI token_request_get_WebAccountProvider( struct web_token_request *iface, IInspectable **value )
{
    if (!value) return E_POINTER;
    if ((*value = iface->provider)) IInspectable_AddRef( *value );
    return S_OK;
}

static HRESULT WINAPI token_request_get_Scope( struct web_token_request *iface, HSTRING *value )
{
    if (!value) return E_POINTER;
    return WindowsDuplicateString( iface->scope, value );
}

static HRESULT WINAPI token_request_get_ClientId( struct web_token_request *iface, HSTRING *value )
{
    if (!value) return E_POINTER;
    return WindowsDuplicateString( iface->client_id, value );
}

static HRESULT WINAPI token_request_get_PromptType( struct web_token_request *iface, INT32 *value )
{
    if (!value) return E_POINTER;
    *value = iface->prompt_type;
    return S_OK;
}

static HRESULT WINAPI token_request_get_Properties( struct web_token_request *iface, IInspectable **value )
{
    if (!value) return E_POINTER;
    IInspectable_AddRef( *value = iface->properties );
    return S_OK;
}

static HRESULT WINAPI token_request3_get_CorrelationId( struct web_token_request *iface, HSTRING *value )
{
    struct web_token_request *impl = token_request_from_request3( iface );
    if (!value) return E_POINTER;
    TRACE( "iface %p, value %s.\n", iface, debugstr_hstring( impl->correlation_id ) );
    return WindowsDuplicateString( impl->correlation_id, value );
}

static HRESULT WINAPI token_request3_put_CorrelationId( struct web_token_request *iface, HSTRING value )
{
    struct web_token_request *impl = token_request_from_request3( iface );
    HSTRING copy;
    HRESULT hr;

    TRACE( "iface %p, value %s.\n", iface, debugstr_hstring( value ) );
    if (FAILED(hr = WindowsDuplicateString( value, &copy ))) return hr;
    WindowsDeleteString( impl->correlation_id );
    impl->correlation_id = copy;
    return S_OK;
}

static const struct web_token_request_vtbl token_request_vtbl =
{
    token_request_QueryInterface, token_request_AddRef, token_request_Release,
    token_request_GetIids, token_request_GetRuntimeClassName, token_request_GetTrustLevel,
    token_request_get_WebAccountProvider, token_request_get_Scope, token_request_get_ClientId,
    token_request_get_PromptType, token_request_get_Properties,
};

static const struct web_token_request3_vtbl token_request3_vtbl =
{
    token_request3_QueryInterface, token_request3_AddRef, token_request3_Release,
    token_request3_GetIids, token_request3_GetRuntimeClassName, token_request3_GetTrustLevel,
    token_request3_get_CorrelationId, token_request3_put_CorrelationId,
};

static HRESULT web_token_request_create( IInspectable *provider, HSTRING scope, HSTRING client_id,
                                         INT32 prompt_type, IInspectable **out )
{
    struct web_token_request *impl;
    HRESULT hr;

    if (!out) return E_POINTER;
    *out = NULL;
    if (!provider) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &token_request_vtbl;
    impl->request3_vtbl = &token_request3_vtbl;
    impl->ref = 1;
    IInspectable_AddRef( impl->provider = provider );
    if (FAILED(hr = string_map_create( &impl->properties )) ||
        FAILED(hr = WindowsDuplicateString( scope, &impl->scope )) ||
        FAILED(hr = WindowsDuplicateString( client_id, &impl->client_id )))
    {
        token_request_Release( impl );
        return hr;
    }
    impl->prompt_type = prompt_type;
    *out = (IInspectable *)impl;
    return S_OK;
}

/* A completed IAsyncOperation<T>.  Typed WinRT async operations share this ABI;
 * the returned pointer is consumed directly by generated callers. */
struct completed_provider_operation
{
    IAsyncOperation_IInspectable IAsyncOperation_IInspectable_iface;
    IAsyncInfo IAsyncInfo_iface;
    LONG ref;
    IInspectable *result;
    IAsyncOperationCompletedHandler_IInspectable *handler;
    BOOL closed;
};

static inline struct completed_provider_operation *impl_from_async_operation( IAsyncOperation_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct completed_provider_operation, IAsyncOperation_IInspectable_iface );
}

static inline struct completed_provider_operation *impl_from_async_info( IAsyncInfo *iface )
{
    return CONTAINING_RECORD( iface, struct completed_provider_operation, IAsyncInfo_iface );
}

static HRESULT WINAPI completed_async_QueryInterface( IAsyncOperation_IInspectable *iface, REFIID iid, void **out )
{
    struct completed_provider_operation *impl = impl_from_async_operation( iface );
    if (!out) return E_POINTER;
    if (IsEqualGUID( iid, &IID_IAsyncInfo )) *out = &impl->IAsyncInfo_iface;
    else
    {
        /* The parameterized IAsyncOperation<WebAccountProvider> IID is not in
         * Wine's current credentials IDL.  This object has the single async
         * operation ABI, so accept the generated operation IID here as well. */
        if (!IsEqualGUID( iid, &IID_IUnknown ) && !IsEqualGUID( iid, &IID_IInspectable ) &&
            !IsEqualGUID( iid, &IID_IAgileObject ))
            TRACE( "accepting parameterized async IID %s.\n", debugstr_guid( iid ) );
        *out = iface;
    }
    TRACE( "async iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), *out );
    InterlockedIncrement( &impl->ref );
    return S_OK;
}

static ULONG WINAPI completed_async_AddRef( IAsyncOperation_IInspectable *iface )
{
    return InterlockedIncrement( &impl_from_async_operation( iface )->ref );
}

static ULONG WINAPI completed_async_Release( IAsyncOperation_IInspectable *iface )
{
    struct completed_provider_operation *impl = impl_from_async_operation( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    if (!ref)
    {
        if (impl->handler) IAsyncOperationCompletedHandler_IInspectable_Release( impl->handler );
        if (impl->result) IInspectable_Release( impl->result );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI completed_async_GetIids( IAsyncOperation_IInspectable *iface, ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IAsyncInfo;
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI completed_async_GetRuntimeClassName( IAsyncOperation_IInspectable *iface, HSTRING *name )
{
    static const WCHAR class_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Security.Credentials.WebAccountProvider>";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI completed_async_GetTrustLevel( IAsyncOperation_IInspectable *iface, TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI completed_async_put_Completed( IAsyncOperation_IInspectable *iface,
        IAsyncOperationCompletedHandler_IInspectable *handler )
{
    struct completed_provider_operation *impl = impl_from_async_operation( iface );
    HRESULT hr;

    TRACE( "async iface %p, handler %p.\n", iface, handler );
    if (impl->closed) return E_ILLEGAL_METHOD_CALL;
    if (impl->handler) return E_ILLEGAL_DELEGATE_ASSIGNMENT;
    if (!handler) return E_POINTER;
    IAsyncOperationCompletedHandler_IInspectable_AddRef( handler );
    impl->handler = handler;
    completed_async_AddRef( iface );
    hr = IAsyncOperationCompletedHandler_IInspectable_Invoke( handler, iface, Completed );
    TRACE( "handler %p returned %#lx.\n", handler, hr );
    completed_async_Release( iface );
    return hr;
}

static HRESULT WINAPI completed_async_get_Completed( IAsyncOperation_IInspectable *iface,
        IAsyncOperationCompletedHandler_IInspectable **handler )
{
    struct completed_provider_operation *impl = impl_from_async_operation( iface );
    if (!handler) return E_POINTER;
    if (impl->closed) return E_ILLEGAL_METHOD_CALL;
    if ((*handler = impl->handler)) IAsyncOperationCompletedHandler_IInspectable_AddRef( *handler );
    return S_OK;
}

static HRESULT WINAPI completed_async_GetResults( IAsyncOperation_IInspectable *iface, IInspectable **result )
{
    struct completed_provider_operation *impl = impl_from_async_operation( iface );
    if (!result) return E_POINTER;
    if (impl->closed) return E_ILLEGAL_METHOD_CALL;
    IInspectable_AddRef( *result = impl->result );
    TRACE( "async iface %p returned result %p.\n", iface, *result );
    return S_OK;
}

static const IAsyncOperation_IInspectableVtbl completed_async_vtbl =
{
    completed_async_QueryInterface, completed_async_AddRef, completed_async_Release,
    completed_async_GetIids, completed_async_GetRuntimeClassName, completed_async_GetTrustLevel,
    completed_async_put_Completed, completed_async_get_Completed, completed_async_GetResults,
};

static HRESULT WINAPI completed_info_QueryInterface( IAsyncInfo *iface, REFIID iid, void **out )
{
    return completed_async_QueryInterface( &impl_from_async_info( iface )->IAsyncOperation_IInspectable_iface, iid, out );
}

static ULONG WINAPI completed_info_AddRef( IAsyncInfo *iface )
{
    return completed_async_AddRef( &impl_from_async_info( iface )->IAsyncOperation_IInspectable_iface );
}

static ULONG WINAPI completed_info_Release( IAsyncInfo *iface )
{
    return completed_async_Release( &impl_from_async_info( iface )->IAsyncOperation_IInspectable_iface );
}

static HRESULT WINAPI completed_info_GetIids( IAsyncInfo *iface, ULONG *count, IID **iids )
{
    return completed_async_GetIids( &impl_from_async_info( iface )->IAsyncOperation_IInspectable_iface, count, iids );
}

static HRESULT WINAPI completed_info_GetRuntimeClassName( IAsyncInfo *iface, HSTRING *name )
{
    return completed_async_GetRuntimeClassName( &impl_from_async_info( iface )->IAsyncOperation_IInspectable_iface, name );
}

static HRESULT WINAPI completed_info_GetTrustLevel( IAsyncInfo *iface, TrustLevel *level )
{
    return completed_async_GetTrustLevel( &impl_from_async_info( iface )->IAsyncOperation_IInspectable_iface, level );
}

static HRESULT WINAPI completed_info_get_Id( IAsyncInfo *iface, UINT32 *id )
{
    if (!id) return E_POINTER;
    if (impl_from_async_info( iface )->closed) return E_ILLEGAL_METHOD_CALL;
    *id = 1;
    return S_OK;
}

static HRESULT WINAPI completed_info_get_Status( IAsyncInfo *iface, AsyncStatus *status )
{
    if (!status) return E_POINTER;
    if (impl_from_async_info( iface )->closed) return E_ILLEGAL_METHOD_CALL;
    *status = Completed;
    return S_OK;
}

static HRESULT WINAPI completed_info_get_ErrorCode( IAsyncInfo *iface, HRESULT *error )
{
    if (!error) return E_POINTER;
    if (impl_from_async_info( iface )->closed) return E_ILLEGAL_METHOD_CALL;
    *error = S_OK;
    return S_OK;
}

static HRESULT WINAPI completed_info_Cancel( IAsyncInfo *iface )
{
    return impl_from_async_info( iface )->closed ? E_ILLEGAL_METHOD_CALL : S_OK;
}

static HRESULT WINAPI completed_info_Close( IAsyncInfo *iface )
{
    impl_from_async_info( iface )->closed = TRUE;
    return S_OK;
}

static const IAsyncInfoVtbl completed_info_vtbl =
{
    completed_info_QueryInterface, completed_info_AddRef, completed_info_Release,
    completed_info_GetIids, completed_info_GetRuntimeClassName, completed_info_GetTrustLevel,
    completed_info_get_Id, completed_info_get_Status, completed_info_get_ErrorCode,
    completed_info_Cancel, completed_info_Close,
};

static HRESULT completed_provider_operation_create( IInspectable *result, IInspectable **out )
{
    struct completed_provider_operation *impl;
    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IAsyncOperation_IInspectable_iface.lpVtbl = &completed_async_vtbl;
    impl->IAsyncInfo_iface.lpVtbl = &completed_info_vtbl;
    impl->ref = 1;
    IInspectable_AddRef( impl->result = result );
    *out = (IInspectable *)&impl->IAsyncOperation_IInspectable_iface;
    return S_OK;
}

/* Minimal empty IVectorView<WebAccount>.  Cached-account enumeration may
 * legitimately succeed with zero accounts; Office uses that outcome to move
 * on to an interactive token request. */
struct empty_account_vector
{
    IVectorView_IInspectable IVectorView_IInspectable_iface;
    LONG ref;
};

static inline struct empty_account_vector *impl_from_account_vector( IVectorView_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct empty_account_vector, IVectorView_IInspectable_iface );
}

static HRESULT WINAPI account_vector_QueryInterface( IVectorView_IInspectable *iface, REFIID iid, void **out )
{
    struct empty_account_vector *impl = impl_from_account_vector( iface );
    if (!out) return E_POINTER;
    /* The parameterized IVectorView<WebAccount> IID is absent from Wine's
     * credentials IDL.  This empty object has no element-specific behavior. */
    if (!IsEqualGUID( iid, &IID_IUnknown ) && !IsEqualGUID( iid, &IID_IInspectable ) &&
        !IsEqualGUID( iid, &IID_IAgileObject ))
        TRACE( "accepting parameterized vector IID %s.\n", debugstr_guid( iid ) );
    *out = iface;
    TRACE( "vector iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), *out );
    InterlockedIncrement( &impl->ref );
    return S_OK;
}

static ULONG WINAPI account_vector_AddRef( IVectorView_IInspectable *iface )
{
    return InterlockedIncrement( &impl_from_account_vector( iface )->ref );
}

static ULONG WINAPI account_vector_Release( IVectorView_IInspectable *iface )
{
    struct empty_account_vector *impl = impl_from_account_vector( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI account_vector_GetIids( IVectorView_IInspectable *iface, ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    *count = 0;
    *iids = NULL;
    return S_OK;
}

static HRESULT WINAPI account_vector_GetRuntimeClassName( IVectorView_IInspectable *iface, HSTRING *name )
{
    static const WCHAR class_name[] = L"Windows.Foundation.Collections.IVectorView`1<Windows.Security.Credentials.WebAccount>";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI account_vector_GetTrustLevel( IVectorView_IInspectable *iface, TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI account_vector_GetAt( IVectorView_IInspectable *iface, UINT32 index, IInspectable **value )
{
    if (!value) return E_POINTER;
    *value = NULL;
    return E_BOUNDS;
}

static HRESULT WINAPI account_vector_get_Size( IVectorView_IInspectable *iface, UINT32 *value )
{
    if (!value) return E_POINTER;
    *value = 0;
    TRACE( "vector iface %p returned size 0.\n", iface );
    return S_OK;
}

static HRESULT WINAPI account_vector_IndexOf( IVectorView_IInspectable *iface, IInspectable *element,
                                               UINT32 *index, boolean *found )
{
    if (!index || !found) return E_POINTER;
    *index = 0;
    *found = FALSE;
    return S_OK;
}

static HRESULT WINAPI account_vector_GetMany( IVectorView_IInspectable *iface, UINT32 start_index,
                                               UINT32 items_size, IInspectable **items, UINT32 *count )
{
    if (!count) return E_POINTER;
    *count = 0;
    return start_index ? E_BOUNDS : S_OK;
}

static const IVectorView_IInspectableVtbl account_vector_vtbl =
{
    account_vector_QueryInterface, account_vector_AddRef, account_vector_Release,
    account_vector_GetIids, account_vector_GetRuntimeClassName, account_vector_GetTrustLevel,
    account_vector_GetAt, account_vector_get_Size, account_vector_IndexOf, account_vector_GetMany,
};

static HRESULT empty_account_vector_create( IInspectable **out )
{
    struct empty_account_vector *impl;
    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IVectorView_IInspectable_iface.lpVtbl = &account_vector_vtbl;
    impl->ref = 1;
    *out = (IInspectable *)&impl->IVectorView_IInspectable_iface;
    return S_OK;
}

static const GUID IID_IFindAllAccountsResult =
    {0xa5812b5d, 0xb72e, 0x420c, {0x86, 0xab, 0xaa, 0xc0, 0xd7, 0xb7, 0x26, 0x1f}};

struct find_all_accounts_result;
struct find_all_accounts_result_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct find_all_accounts_result *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct find_all_accounts_result *);
    ULONG (WINAPI *Release)(struct find_all_accounts_result *);
    HRESULT (WINAPI *GetIids)(struct find_all_accounts_result *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct find_all_accounts_result *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct find_all_accounts_result *, TrustLevel *);
    HRESULT (WINAPI *get_Accounts)(struct find_all_accounts_result *, IInspectable **);
    HRESULT (WINAPI *get_Status)(struct find_all_accounts_result *, INT32 *);
    HRESULT (WINAPI *get_ProviderError)(struct find_all_accounts_result *, IInspectable **);
};

struct find_all_accounts_result
{
    const struct find_all_accounts_result_vtbl *lpVtbl;
    LONG ref;
    IInspectable *accounts;
};

static HRESULT WINAPI find_result_QueryInterface( struct find_all_accounts_result *iface, REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!IsEqualGUID( iid, &IID_IUnknown ) && !IsEqualGUID( iid, &IID_IInspectable ) &&
        !IsEqualGUID( iid, &IID_IAgileObject ) && !IsEqualGUID( iid, &IID_IFindAllAccountsResult ))
        return E_NOINTERFACE;
    *out = iface;
    InterlockedIncrement( &iface->ref );
    return S_OK;
}

static ULONG WINAPI find_result_AddRef( struct find_all_accounts_result *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI find_result_Release( struct find_all_accounts_result *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        IInspectable_Release( iface->accounts );
        free( iface );
    }
    return ref;
}

static HRESULT WINAPI find_result_GetIids( struct find_all_accounts_result *iface, ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IFindAllAccountsResult;
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI find_result_GetRuntimeClassName( struct find_all_accounts_result *iface, HSTRING *name )
{
    static const WCHAR class_name[] = L"Windows.Security.Authentication.Web.Core.FindAllAccountsResult";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI find_result_GetTrustLevel( struct find_all_accounts_result *iface, TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI find_result_get_Accounts( struct find_all_accounts_result *iface, IInspectable **value )
{
    if (!value) return E_POINTER;
    IInspectable_AddRef( *value = iface->accounts );
    return S_OK;
}

static HRESULT WINAPI find_result_get_Status( struct find_all_accounts_result *iface, INT32 *value )
{
    if (!value) return E_POINTER;
    *value = 0; /* FindAllWebAccountsStatus_Success */
    return S_OK;
}

static HRESULT WINAPI find_result_get_ProviderError( struct find_all_accounts_result *iface, IInspectable **value )
{
    if (!value) return E_POINTER;
    *value = NULL;
    return S_OK;
}

static const struct find_all_accounts_result_vtbl find_result_vtbl =
{
    find_result_QueryInterface, find_result_AddRef, find_result_Release,
    find_result_GetIids, find_result_GetRuntimeClassName, find_result_GetTrustLevel,
    find_result_get_Accounts, find_result_get_Status, find_result_get_ProviderError,
};

static HRESULT find_all_accounts_result_create( IInspectable **out )
{
    struct find_all_accounts_result *impl;
    HRESULT hr;

    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &find_result_vtbl;
    impl->ref = 1;
    if (FAILED(hr = empty_account_vector_create( &impl->accounts )))
    {
        free( impl );
        return hr;
    }
    *out = (IInspectable *)impl;
    return S_OK;
}

static const GUID IID_IWebProviderError =
    {0xdb191bb1, 0x50c5, 0x4809, {0x8d, 0xca, 0x09, 0xc9, 0x94, 0x10, 0x24, 0x5c}};

struct web_provider_error;
struct web_provider_error_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_provider_error *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_provider_error *);
    ULONG (WINAPI *Release)(struct web_provider_error *);
    HRESULT (WINAPI *GetIids)(struct web_provider_error *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_provider_error *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_provider_error *, TrustLevel *);
    HRESULT (WINAPI *get_ErrorCode)(struct web_provider_error *, UINT32 *);
    HRESULT (WINAPI *get_ErrorMessage)(struct web_provider_error *, HSTRING *);
    HRESULT (WINAPI *get_Properties)(struct web_provider_error *, IInspectable **);
};

struct web_provider_error
{
    const struct web_provider_error_vtbl *lpVtbl;
    LONG ref;
    UINT32 code;
    HSTRING message;
    IInspectable *properties;
};

static HRESULT WINAPI provider_error_QueryInterface( struct web_provider_error *iface, REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!IsEqualGUID( iid, &IID_IUnknown ) && !IsEqualGUID( iid, &IID_IInspectable ) &&
        !IsEqualGUID( iid, &IID_IAgileObject ) && !IsEqualGUID( iid, &IID_IWebProviderError ))
        return E_NOINTERFACE;
    *out = iface;
    TRACE( "provider error iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), *out );
    InterlockedIncrement( &iface->ref );
    return S_OK;
}

static ULONG WINAPI provider_error_AddRef( struct web_provider_error *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI provider_error_Release( struct web_provider_error *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        WindowsDeleteString( iface->message );
        if (iface->properties) IInspectable_Release( iface->properties );
        free( iface );
    }
    return ref;
}

static HRESULT WINAPI provider_error_GetIids( struct web_provider_error *iface, ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IWebProviderError;
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI provider_error_GetRuntimeClassName( struct web_provider_error *iface, HSTRING *name )
{
    static const WCHAR class_name[] = L"Windows.Security.Authentication.Web.Core.WebProviderError";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI provider_error_GetTrustLevel( struct web_provider_error *iface, TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI provider_error_get_ErrorCode( struct web_provider_error *iface, UINT32 *value )
{
    if (!value) return E_POINTER;
    *value = iface->code;
    TRACE( "provider error iface %p returned code %#x.\n", iface, *value );
    return S_OK;
}

static HRESULT WINAPI provider_error_get_ErrorMessage( struct web_provider_error *iface, HSTRING *value )
{
    if (!value) return E_POINTER;
    TRACE( "provider error iface %p returned message %s.\n", iface, debugstr_hstring( iface->message ) );
    return WindowsDuplicateString( iface->message, value );
}

static HRESULT WINAPI provider_error_get_Properties( struct web_provider_error *iface, IInspectable **value )
{
    if (!value) return E_POINTER;
    IInspectable_AddRef( *value = iface->properties );
    TRACE( "provider error iface %p returned properties %p.\n", iface, *value );
    return S_OK;
}

static const struct web_provider_error_vtbl provider_error_vtbl =
{
    provider_error_QueryInterface, provider_error_AddRef, provider_error_Release,
    provider_error_GetIids, provider_error_GetRuntimeClassName, provider_error_GetTrustLevel,
    provider_error_get_ErrorCode, provider_error_get_ErrorMessage, provider_error_get_Properties,
};

static HRESULT web_provider_error_create( UINT32 code, const WCHAR *message, IInspectable **out )
{
    struct web_provider_error *impl;
    HRESULT hr;

    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &provider_error_vtbl;
    impl->ref = 1;
    impl->code = code;
    if (FAILED(hr = WindowsCreateString( message, wcslen( message ), &impl->message )) ||
        FAILED(hr = string_map_create( &impl->properties )))
    {
        provider_error_Release( impl );
        return hr;
    }
    *out = (IInspectable *)impl;
    return S_OK;
}

static const GUID IID_IWebTokenRequestResult =
    {0xc12a8305, 0xd1f8, 0x4483, {0x8d, 0x54, 0x38, 0xfe, 0x29, 0x27, 0x84, 0xff}};

struct web_token_request_result;
struct web_token_request_result_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_token_request_result *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_token_request_result *);
    ULONG (WINAPI *Release)(struct web_token_request_result *);
    HRESULT (WINAPI *GetIids)(struct web_token_request_result *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_token_request_result *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_token_request_result *, TrustLevel *);
    HRESULT (WINAPI *get_ResponseData)(struct web_token_request_result *, IInspectable **);
    HRESULT (WINAPI *get_ResponseStatus)(struct web_token_request_result *, INT32 *);
    HRESULT (WINAPI *get_ResponseError)(struct web_token_request_result *, IInspectable **);
    HRESULT (WINAPI *InvalidateCacheAsync)(struct web_token_request_result *, IInspectable **);
};

struct web_token_request_result
{
    const struct web_token_request_result_vtbl *lpVtbl;
    LONG ref;
    IInspectable *responses;
    IInspectable *error;
    INT32 status;
};

static HRESULT WINAPI token_result_QueryInterface( struct web_token_request_result *iface, REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!IsEqualGUID( iid, &IID_IUnknown ) && !IsEqualGUID( iid, &IID_IInspectable ) &&
        !IsEqualGUID( iid, &IID_IAgileObject ) && !IsEqualGUID( iid, &IID_IWebTokenRequestResult ))
        return E_NOINTERFACE;
    *out = iface;
    TRACE( "result iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), *out );
    InterlockedIncrement( &iface->ref );
    return S_OK;
}

static ULONG WINAPI token_result_AddRef( struct web_token_request_result *iface )
{
    return InterlockedIncrement( &iface->ref );
}

static ULONG WINAPI token_result_Release( struct web_token_request_result *iface )
{
    ULONG ref = InterlockedDecrement( &iface->ref );
    if (!ref)
    {
        if (iface->responses) IInspectable_Release( iface->responses );
        if (iface->error) IInspectable_Release( iface->error );
        free( iface );
    }
    return ref;
}

static HRESULT WINAPI token_result_GetIids( struct web_token_request_result *iface, ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IWebTokenRequestResult;
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI token_result_GetRuntimeClassName( struct web_token_request_result *iface, HSTRING *name )
{
    static const WCHAR class_name[] = L"Windows.Security.Authentication.Web.Core.WebTokenRequestResult";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI token_result_GetTrustLevel( struct web_token_request_result *iface, TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI token_result_get_ResponseData( struct web_token_request_result *iface, IInspectable **value )
{
    if (!value) return E_POINTER;
    IInspectable_AddRef( *value = iface->responses );
    TRACE( "result iface %p returned response data %p.\n", iface, *value );
    return S_OK;
}

static HRESULT WINAPI token_result_get_ResponseStatus( struct web_token_request_result *iface, INT32 *value )
{
    if (!value) return E_POINTER;
    *value = iface->status;
    TRACE( "result iface %p returned status %d.\n", iface, *value );
    return S_OK;
}

static HRESULT WINAPI token_result_get_ResponseError( struct web_token_request_result *iface, IInspectable **value )
{
    if (!value) return E_POINTER;
    IInspectable_AddRef( *value = iface->error );
    TRACE( "result iface %p returned provider error %p.\n", iface, *value );
    return S_OK;
}

static HRESULT WINAPI token_result_InvalidateCacheAsync( struct web_token_request_result *iface,
                                                         IInspectable **operation )
{
    if (operation) *operation = NULL;
    return E_NOTIMPL;
}

static const struct web_token_request_result_vtbl token_result_vtbl =
{
    token_result_QueryInterface, token_result_AddRef, token_result_Release,
    token_result_GetIids, token_result_GetRuntimeClassName, token_result_GetTrustLevel,
    token_result_get_ResponseData, token_result_get_ResponseStatus,
    token_result_get_ResponseError, token_result_InvalidateCacheAsync,
};

static HRESULT web_token_request_result_create( INT32 status, IInspectable **out )
{
    struct web_token_request_result *impl;
    HRESULT hr;
    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->lpVtbl = &token_result_vtbl;
    impl->ref = 1;
    impl->status = status;
    if (FAILED(hr = empty_account_vector_create( &impl->responses )) ||
        FAILED(hr = web_provider_error_create( 0, L"User interaction required", &impl->error )))
    {
        token_result_Release( impl );
        return hr;
    }
    *out = (IInspectable *)impl;
    return S_OK;
}

struct web_authentication_core_manager_statics;
struct web_authentication_core_manager_statics4;
struct web_authentication_core_manager_interop;

struct web_authentication_core_manager_statics_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_authentication_core_manager_statics *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_authentication_core_manager_statics *);
    ULONG (WINAPI *Release)(struct web_authentication_core_manager_statics *);
    HRESULT (WINAPI *GetIids)(struct web_authentication_core_manager_statics *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_authentication_core_manager_statics *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_authentication_core_manager_statics *, TrustLevel *);
    HRESULT (WINAPI *GetTokenSilentlyAsync)(struct web_authentication_core_manager_statics *, IInspectable *, IInspectable **);
    HRESULT (WINAPI *GetTokenSilentlyWithWebAccountAsync)(struct web_authentication_core_manager_statics *, IInspectable *, IInspectable *, IInspectable **);
    HRESULT (WINAPI *RequestTokenAsync)(struct web_authentication_core_manager_statics *, IInspectable *, IInspectable **);
    HRESULT (WINAPI *RequestTokenWithWebAccountAsync)(struct web_authentication_core_manager_statics *, IInspectable *, IInspectable *, IInspectable **);
    HRESULT (WINAPI *FindAccountAsync)(struct web_authentication_core_manager_statics *, IInspectable *, HSTRING, IInspectable **);
    HRESULT (WINAPI *FindAccountProviderAsync)(struct web_authentication_core_manager_statics *, HSTRING, IInspectable **);
    HRESULT (WINAPI *FindAccountProviderWithAuthorityAsync)(struct web_authentication_core_manager_statics *, HSTRING, HSTRING, IInspectable **);
};

struct web_authentication_core_manager_statics4_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_authentication_core_manager_statics4 *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_authentication_core_manager_statics4 *);
    ULONG (WINAPI *Release)(struct web_authentication_core_manager_statics4 *);
    HRESULT (WINAPI *GetIids)(struct web_authentication_core_manager_statics4 *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_authentication_core_manager_statics4 *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_authentication_core_manager_statics4 *, TrustLevel *);
    HRESULT (WINAPI *FindAllAccountsAsync)(struct web_authentication_core_manager_statics4 *, IInspectable *, IInspectable **);
    HRESULT (WINAPI *FindAllAccountsWithClientIdAsync)(struct web_authentication_core_manager_statics4 *, IInspectable *, HSTRING, IInspectable **);
    HRESULT (WINAPI *FindSystemAccountProviderAsync)(struct web_authentication_core_manager_statics4 *, HSTRING, IInspectable **);
    HRESULT (WINAPI *FindSystemAccountProviderWithAuthorityAsync)(struct web_authentication_core_manager_statics4 *, HSTRING, HSTRING, IInspectable **);
    HRESULT (WINAPI *FindSystemAccountProviderWithAuthorityForUserAsync)(struct web_authentication_core_manager_statics4 *, HSTRING, HSTRING, IInspectable *, IInspectable **);
};

struct web_authentication_core_manager_interop_vtbl
{
    HRESULT (WINAPI *QueryInterface)(struct web_authentication_core_manager_interop *, REFIID, void **);
    ULONG (WINAPI *AddRef)(struct web_authentication_core_manager_interop *);
    ULONG (WINAPI *Release)(struct web_authentication_core_manager_interop *);
    HRESULT (WINAPI *GetIids)(struct web_authentication_core_manager_interop *, ULONG *, IID **);
    HRESULT (WINAPI *GetRuntimeClassName)(struct web_authentication_core_manager_interop *, HSTRING *);
    HRESULT (WINAPI *GetTrustLevel)(struct web_authentication_core_manager_interop *, TrustLevel *);
    HRESULT (WINAPI *RequestTokenForWindowAsync)(struct web_authentication_core_manager_interop *, HWND,
                                                 IInspectable *, REFIID, void **);
    HRESULT (WINAPI *RequestTokenWithWebAccountForWindowAsync)(struct web_authentication_core_manager_interop *,
                                                               HWND, IInspectable *, IInspectable *, REFIID,
                                                               void **);
};

struct web_authentication_core_manager_statics
{
    const struct web_authentication_core_manager_statics_vtbl *lpVtbl;
};

struct web_authentication_core_manager_statics4
{
    const struct web_authentication_core_manager_statics4_vtbl *lpVtbl;
};

struct web_authentication_core_manager_interop
{
    const struct web_authentication_core_manager_interop_vtbl *lpVtbl;
};

struct authenticator_statics
{
    IActivationFactory IActivationFactory_iface;
    IOnlineIdSystemAuthenticatorStatics IOnlineIdSystemAuthenticatorStatics_iface;
    struct web_authentication_core_manager_statics IWebAuthenticationCoreManagerStatics_iface;
    struct web_authentication_core_manager_statics4 IWebAuthenticationCoreManagerStatics4_iface;
    struct web_authentication_core_manager_interop IWebAuthenticationCoreManagerInterop_iface;
    struct web_token_request_factory IWebTokenRequestFactory_iface;
    LONG ref;
};

static inline struct authenticator_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct authenticator_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct authenticator_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IOnlineIdSystemAuthenticatorStatics ))
    {
        *out = &impl->IOnlineIdSystemAuthenticatorStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IWebAuthenticationCoreManagerStatics ))
    {
        *out = &impl->IWebAuthenticationCoreManagerStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IWebAuthenticationCoreManagerStatics4 ))
    {
        *out = &impl->IWebAuthenticationCoreManagerStatics4_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IWebAuthenticationCoreManagerInterop ))
    {
        *out = &impl->IWebAuthenticationCoreManagerInterop_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IWebTokenRequestFactory ))
    {
        *out = &impl->IWebTokenRequestFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct authenticator_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct authenticator_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

static inline struct authenticator_statics *impl_from_token_request_factory( struct web_token_request_factory *iface )
{
    return CONTAINING_RECORD( iface, struct authenticator_statics, IWebTokenRequestFactory_iface );
}

static HRESULT WINAPI token_request_factory_QueryInterface( struct web_token_request_factory *iface,
                                                             REFIID iid, void **out )
{
    struct authenticator_statics *impl = impl_from_token_request_factory( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG WINAPI token_request_factory_AddRef( struct web_token_request_factory *iface )
{
    struct authenticator_statics *impl = impl_from_token_request_factory( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG WINAPI token_request_factory_Release( struct web_token_request_factory *iface )
{
    struct authenticator_statics *impl = impl_from_token_request_factory( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI token_request_factory_GetIids( struct web_token_request_factory *iface,
                                                      ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IWebTokenRequestFactory;
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI token_request_factory_GetRuntimeClassName( struct web_token_request_factory *iface,
                                                                 HSTRING *name )
{
    static const WCHAR class_name[] = L"Windows.Security.Authentication.Web.Core.WebTokenRequest";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI token_request_factory_GetTrustLevel( struct web_token_request_factory *iface,
                                                           TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI token_request_factory_Create( struct web_token_request_factory *iface,
        IInspectable *provider, HSTRING scope, HSTRING client_id, IInspectable **request )
{
    TRACE( "iface %p, provider %p, scope %s, client_id %s, request %p.\n", iface, provider,
           debugstr_hstring( scope ), debugstr_hstring( client_id ), request );
    return web_token_request_create( provider, scope, client_id, 0, request );
}

static HRESULT WINAPI token_request_factory_CreateWithPromptType( struct web_token_request_factory *iface,
        IInspectable *provider, HSTRING scope, HSTRING client_id, INT32 prompt_type, IInspectable **request )
{
    TRACE( "iface %p, provider %p, scope %s, client_id %s, prompt_type %d, request %p.\n", iface, provider,
           debugstr_hstring( scope ), debugstr_hstring( client_id ), prompt_type, request );
    return web_token_request_create( provider, scope, client_id, prompt_type, request );
}

static HRESULT WINAPI token_request_factory_CreateWithProvider( struct web_token_request_factory *iface,
        IInspectable *provider, IInspectable **request )
{
    TRACE( "iface %p, provider %p, request %p.\n", iface, provider, request );
    return web_token_request_create( provider, NULL, NULL, 0, request );
}

static HRESULT WINAPI token_request_factory_CreateWithScope( struct web_token_request_factory *iface,
        IInspectable *provider, HSTRING scope, IInspectable **request )
{
    TRACE( "iface %p, provider %p, scope %s, request %p.\n", iface, provider, debugstr_hstring( scope ), request );
    return web_token_request_create( provider, scope, NULL, 0, request );
}

static const struct web_token_request_factory_vtbl token_request_factory_vtbl =
{
    token_request_factory_QueryInterface, token_request_factory_AddRef, token_request_factory_Release,
    token_request_factory_GetIids, token_request_factory_GetRuntimeClassName, token_request_factory_GetTrustLevel,
    token_request_factory_Create, token_request_factory_CreateWithPromptType,
    token_request_factory_CreateWithProvider, token_request_factory_CreateWithScope,
};

static inline struct authenticator_statics *impl_from_web_manager_statics(
        struct web_authentication_core_manager_statics *iface )
{
    return CONTAINING_RECORD( iface, struct authenticator_statics, IWebAuthenticationCoreManagerStatics_iface );
}

static inline struct authenticator_statics *impl_from_web_manager_statics4(
        struct web_authentication_core_manager_statics4 *iface )
{
    return CONTAINING_RECORD( iface, struct authenticator_statics, IWebAuthenticationCoreManagerStatics4_iface );
}

static HRESULT WINAPI web_manager_statics_QueryInterface( struct web_authentication_core_manager_statics *iface,
                                                           REFIID iid, void **out )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG WINAPI web_manager_statics_AddRef( struct web_authentication_core_manager_statics *iface )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG WINAPI web_manager_statics_Release( struct web_authentication_core_manager_statics *iface )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI web_manager_statics_GetIids( struct web_authentication_core_manager_statics *iface,
                                                    ULONG *iid_count, IID **iids )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics( iface );
    return IActivationFactory_GetIids( &impl->IActivationFactory_iface, iid_count, iids );
}

static HRESULT WINAPI web_manager_statics_GetRuntimeClassName( struct web_authentication_core_manager_statics *iface,
                                                               HSTRING *class_name )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics( iface );
    return IActivationFactory_GetRuntimeClassName( &impl->IActivationFactory_iface, class_name );
}

static HRESULT WINAPI web_manager_statics_GetTrustLevel( struct web_authentication_core_manager_statics *iface,
                                                         TrustLevel *trust_level )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics( iface );
    return IActivationFactory_GetTrustLevel( &impl->IActivationFactory_iface, trust_level );
}

#define WEB_MANAGER_ASYNC_STUB(name, params, args) \
    static HRESULT WINAPI name params \
    { \
        FIXME args; \
        if (operation) *operation = NULL; \
        return E_NOTIMPL; \
    }

static HRESULT WINAPI web_manager_GetTokenSilentlyAsync(
        struct web_authentication_core_manager_statics *iface, IInspectable *request, IInspectable **operation )
{
    IInspectable *result;
    HRESULT hr;

    TRACE( "iface %p, request %p, operation %p.\n", iface, request, operation );
    if (!operation) return E_POINTER;
    *operation = NULL;
    /* There is no cached Wine WAM account, so the silent request must direct
     * the caller to its interactive path. */
    if (FAILED(hr = web_token_request_result_create( 3, &result ))) return hr;
    hr = completed_provider_operation_create( result, operation );
    IInspectable_Release( result );
    return hr;
}
WEB_MANAGER_ASYNC_STUB( web_manager_GetTokenSilentlyWithWebAccountAsync,
        (struct web_authentication_core_manager_statics *iface, IInspectable *request, IInspectable *account,
         IInspectable **operation),
        ("iface %p, request %p, account %p, operation %p stub!\n", iface, request, account, operation) )
static HRESULT create_interactive_token_operation( INT32 status, REFIID iid, void **out )
{
    IInspectable *result = NULL, *operation = NULL;
    HRESULT hr;

    if (!out) return E_POINTER;
    *out = NULL;
    if (FAILED(hr = web_token_request_result_create( status, &result ))) return hr;
    hr = completed_provider_operation_create( result, &operation );
    IInspectable_Release( result );
    if (FAILED(hr)) return hr;
    hr = IInspectable_QueryInterface( operation, iid, out );
    IInspectable_Release( operation );
    return hr;
}

static HRESULT WINAPI web_manager_RequestTokenAsync(
        struct web_authentication_core_manager_statics *iface, IInspectable *request, IInspectable **operation )
{
    TRACE( "iface %p, request %p, operation %p.\n", iface, request, operation );
    return create_interactive_token_operation( 4, &IID_IInspectable, (void **)operation );
}

static HRESULT WINAPI web_manager_RequestTokenWithWebAccountAsync(
        struct web_authentication_core_manager_statics *iface, IInspectable *request, IInspectable *account,
        IInspectable **operation )
{
    TRACE( "iface %p, request %p, account %p, operation %p.\n", iface, request, account, operation );
    return create_interactive_token_operation( 4, &IID_IInspectable, (void **)operation );
}
WEB_MANAGER_ASYNC_STUB( web_manager_FindAccountAsync,
        (struct web_authentication_core_manager_statics *iface, IInspectable *provider, HSTRING id,
         IInspectable **operation),
        ("iface %p, provider %p, id %s, operation %p stub!\n", iface, provider, debugstr_hstring( id ), operation) )
static HRESULT WINAPI web_manager_FindAccountProviderAsync(
        struct web_authentication_core_manager_statics *iface, HSTRING id, IInspectable **operation )
{
    IInspectable *provider;
    HSTRING authority = NULL;
    HRESULT hr;

    TRACE( "iface %p, id %s, operation %p.\n", iface, debugstr_hstring( id ), operation );
    if (!operation) return E_POINTER;
    *operation = NULL;
    if (FAILED(hr = WindowsCreateString( NULL, 0, &authority ))) return hr;
    if (SUCCEEDED(hr = web_account_provider_create( id, authority, &provider )))
    {
        hr = completed_provider_operation_create( provider, operation );
        IInspectable_Release( provider );
    }
    WindowsDeleteString( authority );
    return hr;
}
static HRESULT WINAPI web_manager_FindAccountProviderWithAuthorityAsync(
        struct web_authentication_core_manager_statics *iface, HSTRING id, HSTRING authority,
        IInspectable **operation )
{
    IInspectable *provider;
    HRESULT hr;

    TRACE( "iface %p, id %s, authority %s, operation %p.\n", iface, debugstr_hstring( id ),
           debugstr_hstring( authority ), operation );
    if (!operation) return E_POINTER;
    *operation = NULL;
    if (FAILED(hr = web_account_provider_create( id, authority, &provider ))) return hr;
    hr = completed_provider_operation_create( provider, operation );
    IInspectable_Release( provider );
    return hr;
}

static const struct web_authentication_core_manager_statics_vtbl web_manager_statics_vtbl =
{
    web_manager_statics_QueryInterface,
    web_manager_statics_AddRef,
    web_manager_statics_Release,
    web_manager_statics_GetIids,
    web_manager_statics_GetRuntimeClassName,
    web_manager_statics_GetTrustLevel,
    web_manager_GetTokenSilentlyAsync,
    web_manager_GetTokenSilentlyWithWebAccountAsync,
    web_manager_RequestTokenAsync,
    web_manager_RequestTokenWithWebAccountAsync,
    web_manager_FindAccountAsync,
    web_manager_FindAccountProviderAsync,
    web_manager_FindAccountProviderWithAuthorityAsync,
};

static inline struct authenticator_statics *impl_from_web_manager_interop(
        struct web_authentication_core_manager_interop *iface )
{
    return CONTAINING_RECORD( iface, struct authenticator_statics, IWebAuthenticationCoreManagerInterop_iface );
}

static HRESULT WINAPI web_manager_interop_QueryInterface( struct web_authentication_core_manager_interop *iface,
                                                           REFIID iid, void **out )
{
    struct authenticator_statics *impl = impl_from_web_manager_interop( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG WINAPI web_manager_interop_AddRef( struct web_authentication_core_manager_interop *iface )
{
    struct authenticator_statics *impl = impl_from_web_manager_interop( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG WINAPI web_manager_interop_Release( struct web_authentication_core_manager_interop *iface )
{
    struct authenticator_statics *impl = impl_from_web_manager_interop( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI web_manager_interop_GetIids( struct web_authentication_core_manager_interop *iface,
                                                    ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IWebAuthenticationCoreManagerInterop;
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI web_manager_interop_GetRuntimeClassName(
        struct web_authentication_core_manager_interop *iface, HSTRING *name )
{
    static const WCHAR class_name[] =
        L"Windows.Security.Authentication.Web.Core.WebAuthenticationCoreManager";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI web_manager_interop_GetTrustLevel( struct web_authentication_core_manager_interop *iface,
                                                          TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI web_manager_interop_RequestTokenForWindowAsync(
        struct web_authentication_core_manager_interop *iface, HWND window, IInspectable *request,
        REFIID iid, void **operation )
{
    TRACE( "iface %p, window %p, request %p, iid %s, operation %p.\n", iface, window, request,
           debugstr_guid( iid ), operation );
    /* UserInteractionRequired is only valid for the silent API.  Report that
     * this Wine broker has no interactive account provider and let MSAL choose
     * its non-WAM browser path. */
    return create_interactive_token_operation( 4, iid, operation );
}

static HRESULT WINAPI web_manager_interop_RequestTokenWithWebAccountForWindowAsync(
        struct web_authentication_core_manager_interop *iface, HWND window, IInspectable *request,
        IInspectable *account, REFIID iid, void **operation )
{
    TRACE( "iface %p, window %p, request %p, account %p, iid %s, operation %p.\n", iface, window,
           request, account, debugstr_guid( iid ), operation );
    return create_interactive_token_operation( 4, iid, operation );
}

static const struct web_authentication_core_manager_interop_vtbl web_manager_interop_vtbl =
{
    web_manager_interop_QueryInterface,
    web_manager_interop_AddRef,
    web_manager_interop_Release,
    web_manager_interop_GetIids,
    web_manager_interop_GetRuntimeClassName,
    web_manager_interop_GetTrustLevel,
    web_manager_interop_RequestTokenForWindowAsync,
    web_manager_interop_RequestTokenWithWebAccountForWindowAsync,
};

static HRESULT WINAPI web_manager_statics4_QueryInterface( struct web_authentication_core_manager_statics4 *iface,
                                                            REFIID iid, void **out )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics4( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG WINAPI web_manager_statics4_AddRef( struct web_authentication_core_manager_statics4 *iface )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics4( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG WINAPI web_manager_statics4_Release( struct web_authentication_core_manager_statics4 *iface )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics4( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT WINAPI web_manager_statics4_GetIids( struct web_authentication_core_manager_statics4 *iface,
                                                     ULONG *iid_count, IID **iids )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics4( iface );
    return IActivationFactory_GetIids( &impl->IActivationFactory_iface, iid_count, iids );
}

static HRESULT WINAPI web_manager_statics4_GetRuntimeClassName( struct web_authentication_core_manager_statics4 *iface,
                                                                HSTRING *class_name )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics4( iface );
    return IActivationFactory_GetRuntimeClassName( &impl->IActivationFactory_iface, class_name );
}

static HRESULT WINAPI web_manager_statics4_GetTrustLevel( struct web_authentication_core_manager_statics4 *iface,
                                                          TrustLevel *trust_level )
{
    struct authenticator_statics *impl = impl_from_web_manager_statics4( iface );
    return IActivationFactory_GetTrustLevel( &impl->IActivationFactory_iface, trust_level );
}

#define WEB_MANAGER4_ASYNC_STUB(name, params, args) \
    static HRESULT WINAPI name params \
    { \
        FIXME args; \
        if (operation) *operation = NULL; \
        return E_NOTIMPL; \
    }

WEB_MANAGER4_ASYNC_STUB( web_manager_FindAllAccountsAsync,
        (struct web_authentication_core_manager_statics4 *iface, IInspectable *provider, IInspectable **operation),
        ("iface %p, provider %p, operation %p stub!\n", iface, provider, operation) )
static HRESULT WINAPI web_manager_FindAllAccountsWithClientIdAsync(
        struct web_authentication_core_manager_statics4 *iface, IInspectable *provider, HSTRING client_id,
        IInspectable **operation )
{
    IInspectable *result;
    HRESULT hr;

    TRACE( "iface %p, provider %p, client_id %s, operation %p.\n", iface, provider,
           debugstr_hstring( client_id ), operation );
    if (!operation) return E_POINTER;
    *operation = NULL;
    if (FAILED(hr = find_all_accounts_result_create( &result ))) return hr;
    hr = completed_provider_operation_create( result, operation );
    IInspectable_Release( result );
    return hr;
}
WEB_MANAGER4_ASYNC_STUB( web_manager_FindSystemAccountProviderAsync,
        (struct web_authentication_core_manager_statics4 *iface, HSTRING id, IInspectable **operation),
        ("iface %p, id %s, operation %p stub!\n", iface, debugstr_hstring( id ), operation) )
WEB_MANAGER4_ASYNC_STUB( web_manager_FindSystemAccountProviderWithAuthorityAsync,
        (struct web_authentication_core_manager_statics4 *iface, HSTRING id, HSTRING authority,
         IInspectable **operation),
        ("iface %p, id %s, authority %s, operation %p stub!\n", iface, debugstr_hstring( id ),
         debugstr_hstring( authority ), operation) )
WEB_MANAGER4_ASYNC_STUB( web_manager_FindSystemAccountProviderWithAuthorityForUserAsync,
        (struct web_authentication_core_manager_statics4 *iface, HSTRING id, HSTRING authority,
         IInspectable *user, IInspectable **operation),
        ("iface %p, id %s, authority %s, user %p, operation %p stub!\n", iface, debugstr_hstring( id ),
         debugstr_hstring( authority ), user, operation) )

static const struct web_authentication_core_manager_statics4_vtbl web_manager_statics4_vtbl =
{
    web_manager_statics4_QueryInterface,
    web_manager_statics4_AddRef,
    web_manager_statics4_Release,
    web_manager_statics4_GetIids,
    web_manager_statics4_GetRuntimeClassName,
    web_manager_statics4_GetTrustLevel,
    web_manager_FindAllAccountsAsync,
    web_manager_FindAllAccountsWithClientIdAsync,
    web_manager_FindSystemAccountProviderAsync,
    web_manager_FindSystemAccountProviderWithAuthorityAsync,
    web_manager_FindSystemAccountProviderWithAuthorityForUserAsync,
};

struct authenticator
{
    IOnlineIdSystemAuthenticatorForUser IOnlineIdSystemAuthenticatorForUser_iface;
    LONG ref;
    GUID application_id;
};

struct onlineid_ticket_result
{
    IOnlineIdSystemTicketResult IOnlineIdSystemTicketResult_iface;
    LONG ref;
    OnlineIdSystemTicketStatus status;
    HRESULT extended_error;
};

static inline struct onlineid_ticket_result *impl_from_IOnlineIdSystemTicketResult( IOnlineIdSystemTicketResult *iface )
{
    return CONTAINING_RECORD( iface, struct onlineid_ticket_result, IOnlineIdSystemTicketResult_iface );
}

static HRESULT WINAPI onlineid_ticket_result_QueryInterface( IOnlineIdSystemTicketResult *iface,
                                                              REFIID iid, void **out )
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!IsEqualGUID( iid, &IID_IUnknown ) && !IsEqualGUID( iid, &IID_IInspectable ) &&
        !IsEqualGUID( iid, &IID_IAgileObject ) && !IsEqualGUID( iid, &IID_IOnlineIdSystemTicketResult ))
        return E_NOINTERFACE;
    *out = iface;
    IOnlineIdSystemTicketResult_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI onlineid_ticket_result_AddRef( IOnlineIdSystemTicketResult *iface )
{
    return InterlockedIncrement( &impl_from_IOnlineIdSystemTicketResult( iface )->ref );
}

static ULONG WINAPI onlineid_ticket_result_Release( IOnlineIdSystemTicketResult *iface )
{
    struct onlineid_ticket_result *impl = impl_from_IOnlineIdSystemTicketResult( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI onlineid_ticket_result_GetIids( IOnlineIdSystemTicketResult *iface,
                                                       ULONG *count, IID **iids )
{
    if (!count || !iids) return E_POINTER;
    if (!(*iids = CoTaskMemAlloc( sizeof(**iids) ))) return E_OUTOFMEMORY;
    (*iids)[0] = IID_IOnlineIdSystemTicketResult;
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI onlineid_ticket_result_GetRuntimeClassName( IOnlineIdSystemTicketResult *iface,
                                                                  HSTRING *name )
{
    static const WCHAR class_name[] =
        L"Windows.Security.Authentication.OnlineId.OnlineIdSystemTicketResult";
    if (!name) return E_POINTER;
    return WindowsCreateString( class_name, ARRAY_SIZE(class_name) - 1, name );
}

static HRESULT WINAPI onlineid_ticket_result_GetTrustLevel( IOnlineIdSystemTicketResult *iface,
                                                             TrustLevel *level )
{
    if (!level) return E_POINTER;
    *level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI onlineid_ticket_result_get_Identity( IOnlineIdSystemTicketResult *iface,
                                                            IOnlineIdSystemIdentity **value )
{
    if (!value) return E_POINTER;
    *value = NULL;
    return S_OK;
}

static HRESULT WINAPI onlineid_ticket_result_get_Status( IOnlineIdSystemTicketResult *iface,
                                                          OnlineIdSystemTicketStatus *value )
{
    struct onlineid_ticket_result *impl = impl_from_IOnlineIdSystemTicketResult( iface );
    if (!value) return E_POINTER;
    *value = impl->status;
    TRACE( "result %p returned status %u.\n", iface, *value );
    return S_OK;
}

static HRESULT WINAPI onlineid_ticket_result_get_ExtendedError( IOnlineIdSystemTicketResult *iface,
                                                                 HRESULT *value )
{
    struct onlineid_ticket_result *impl = impl_from_IOnlineIdSystemTicketResult( iface );
    if (!value) return E_POINTER;
    *value = impl->extended_error;
    TRACE( "result %p returned extended error %#lx.\n", iface, *value );
    return S_OK;
}

static const struct IOnlineIdSystemTicketResultVtbl onlineid_ticket_result_vtbl =
{
    onlineid_ticket_result_QueryInterface,
    onlineid_ticket_result_AddRef,
    onlineid_ticket_result_Release,
    onlineid_ticket_result_GetIids,
    onlineid_ticket_result_GetRuntimeClassName,
    onlineid_ticket_result_GetTrustLevel,
    onlineid_ticket_result_get_Identity,
    onlineid_ticket_result_get_Status,
    onlineid_ticket_result_get_ExtendedError,
};

static HRESULT onlineid_ticket_result_create( OnlineIdSystemTicketStatus status, HRESULT extended_error,
                                              IInspectable **out )
{
    struct onlineid_ticket_result *impl;

    if (!out) return E_POINTER;
    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IOnlineIdSystemTicketResult_iface.lpVtbl = &onlineid_ticket_result_vtbl;
    impl->ref = 1;
    impl->status = status;
    impl->extended_error = extended_error;
    *out = (IInspectable *)&impl->IOnlineIdSystemTicketResult_iface;
    return S_OK;
}

static inline struct authenticator *impl_from_IOnlineIdSystemAuthenticatorForUser( IOnlineIdSystemAuthenticatorForUser *iface )
{
    return CONTAINING_RECORD( iface, struct authenticator, IOnlineIdSystemAuthenticatorForUser_iface );
}

static HRESULT WINAPI authenticator_QueryInterface( IOnlineIdSystemAuthenticatorForUser *iface, REFIID iid, void **out )
{
    struct authenticator *impl = impl_from_IOnlineIdSystemAuthenticatorForUser( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IOnlineIdSystemAuthenticatorForUser ))
    {
        *out = &impl->IOnlineIdSystemAuthenticatorForUser_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI authenticator_AddRef( IOnlineIdSystemAuthenticatorForUser *iface )
{
    struct authenticator *impl = impl_from_IOnlineIdSystemAuthenticatorForUser( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI authenticator_Release( IOnlineIdSystemAuthenticatorForUser *iface )
{
    struct authenticator *impl = impl_from_IOnlineIdSystemAuthenticatorForUser( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI authenticator_GetIids( IOnlineIdSystemAuthenticatorForUser *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI authenticator_GetRuntimeClassName( IOnlineIdSystemAuthenticatorForUser *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI authenticator_GetTrustLevel( IOnlineIdSystemAuthenticatorForUser *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI authenticator_GetTicketAsync( IOnlineIdSystemAuthenticatorForUser *iface, IOnlineIdServiceTicketRequest *request,
                                                    IAsyncOperation_OnlineIdSystemTicketResult **operation )
{
    IInspectable *result;
    HRESULT hr;

    TRACE( "iface %p, request %p, operation %p.\n", iface, request, operation );
    if (!operation) return E_POINTER;
    *operation = NULL;
    if (!request) return E_INVALIDARG;
    if (FAILED(hr = onlineid_ticket_result_create( OnlineIdSystemTicketStatus_ServiceConnectionError,
                                                   HRESULT_FROM_WIN32( ERROR_NETWORK_UNREACHABLE ), &result )))
        return hr;
    hr = completed_provider_operation_create( result, (IInspectable **)operation );
    IInspectable_Release( result );
    return hr;
}

static HRESULT WINAPI authenticator_put_ApplicationId( IOnlineIdSystemAuthenticatorForUser *iface, GUID value )
{
    struct authenticator *impl = impl_from_IOnlineIdSystemAuthenticatorForUser( iface );
    TRACE( "iface %p, value %s.\n", iface, debugstr_guid( &value ) );
    impl->application_id = value;
    return S_OK;
}

static HRESULT WINAPI authenticator_get_ApplicationId( IOnlineIdSystemAuthenticatorForUser *iface, GUID *value )
{
    struct authenticator *impl = impl_from_IOnlineIdSystemAuthenticatorForUser( iface );
    if (!value) return E_POINTER;
    *value = impl->application_id;
    TRACE( "iface %p, value %s.\n", iface, debugstr_guid( value ) );
    return S_OK;
}

static HRESULT WINAPI authenticator_get_User( IOnlineIdSystemAuthenticatorForUser *iface, __x_ABI_CWindows_CSystem_CIUser **user )
{
    FIXME( "iface %p, user %p stub!\n", iface, user );
    return E_NOTIMPL;
}

static const struct IOnlineIdSystemAuthenticatorForUserVtbl authenticator_vtbl =
{
    authenticator_QueryInterface,
    authenticator_AddRef,
    authenticator_Release,
    /* IInspectable methods */
    authenticator_GetIids,
    authenticator_GetRuntimeClassName,
    authenticator_GetTrustLevel,
    /* IOnlineIdSystemAuthenticatorForUser methods */
    authenticator_GetTicketAsync,
    authenticator_put_ApplicationId,
    authenticator_get_ApplicationId,
    authenticator_get_User,
};

DEFINE_IINSPECTABLE( authenticator_statics, IOnlineIdSystemAuthenticatorStatics, struct authenticator_statics, IActivationFactory_iface )

static HRESULT WINAPI authenticator_statics_get_Default( IOnlineIdSystemAuthenticatorStatics *iface,
                                                         IOnlineIdSystemAuthenticatorForUser **value )
{
    struct authenticator *impl;

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IOnlineIdSystemAuthenticatorForUser_iface.lpVtbl = &authenticator_vtbl;
    impl->ref = 1;

    *value = &impl->IOnlineIdSystemAuthenticatorForUser_iface;
    TRACE( "created IOnlineIdSystemAuthenticatorForUser %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI authenticator_statics_GetForUser( IOnlineIdSystemAuthenticatorStatics *iface, __x_ABI_CWindows_CSystem_CIUser *user,
                                                        IOnlineIdSystemAuthenticatorForUser **value )
{
    FIXME( "iface %p, user %p, value %p stub!\n", iface, user, value );
    return E_NOTIMPL;
}

static const struct IOnlineIdSystemAuthenticatorStaticsVtbl authenticator_statics_vtbl =
{
    authenticator_statics_QueryInterface,
    authenticator_statics_AddRef,
    authenticator_statics_Release,
    /* IInspectable methods */
    authenticator_statics_GetIids,
    authenticator_statics_GetRuntimeClassName,
    authenticator_statics_GetTrustLevel,
    /* IOnlineIdSystemAuthenticatorStatics methods */
    authenticator_statics_get_Default,
    authenticator_statics_GetForUser,
};

static struct authenticator_statics authenticator_statics =
{
    {&factory_vtbl},
    {&authenticator_statics_vtbl},
    {&web_manager_statics_vtbl},
    {&web_manager_statics4_vtbl},
    {&web_manager_interop_vtbl},
    {&token_request_factory_vtbl},
    1,
};

IActivationFactory *authenticator_factory = &authenticator_statics.IActivationFactory_iface;
