/* WinRT Windows.Data.Json.JsonArray Implementation
 *
 * Copyright (C) 2026 Olivia Ryan
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
#include <stdint.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(web);

struct json_array
{
    IJsonArray IJsonArray_iface;
    IJsonValue IJsonValue_iface;
    IVector_IJsonValue IVector_IJsonValue_iface;
    LONG ref;
    IJsonValue **elements;
    ULONG capacity;
    ULONG length;
};

static inline struct json_array *impl_from_IJsonValue( IJsonValue *iface )
{
    return CONTAINING_RECORD( iface, struct json_array, IJsonValue_iface );
}

static inline struct json_array *impl_from_IVector_IJsonValue( IVector_IJsonValue *iface )
{
    return CONTAINING_RECORD( iface, struct json_array, IVector_IJsonValue_iface );
}

static inline struct json_array *impl_from_IJsonArray( IJsonArray *iface )
{
    return CONTAINING_RECORD( iface, struct json_array, IJsonArray_iface );
}

HRESULT json_array_push( IJsonArray *iface, IJsonValue *value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (impl->length == impl->capacity)
    {
        UINT32 capacity = max( 32, impl->capacity * 3 / 2 );
        IJsonValue **new = impl->elements;
        if (!(new = realloc( new, capacity * sizeof(*new) ))) return E_OUTOFMEMORY;
        impl->elements = new;
        impl->capacity = capacity;
    }

    impl->elements[impl->length++] = value;
    IJsonValue_AddRef( value );
    return S_OK;
}

static HRESULT WINAPI json_array_QueryInterface( IJsonArray *iface, REFIID iid, void **out )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IJsonArray ))
    {
        *out = &impl->IJsonArray_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IVector_IJsonValue ))
    {
        *out = &impl->IVector_IJsonValue_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IJsonValue ))
    {
        *out = &impl->IJsonValue_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI json_array_AddRef( IJsonArray *iface )
{
    struct json_array *impl = impl_from_IJsonArray( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI json_array_Release( IJsonArray *iface )
{
    struct json_array *impl = impl_from_IJsonArray( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        for (UINT32 i = 0; i < impl->length; i++)
            IJsonValue_Release( impl->elements[i] );

        free( impl->elements );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI json_array_GetIids( IJsonArray *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_array_GetRuntimeClassName( IJsonArray *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_array_GetTrustLevel( IJsonArray *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_array_GetObjectAt( IJsonArray *iface, UINT32 index, IJsonObject **value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetObject( impl->elements[index], value );
}

static HRESULT WINAPI json_array_GetArrayAt( IJsonArray *iface, UINT32 index, IJsonArray **value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetArray( impl->elements[index], value );
}

static HRESULT WINAPI json_array_GetStringAt( IJsonArray *iface, UINT32 index, HSTRING *value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetString( impl->elements[index], value );
}

static HRESULT WINAPI json_array_GetNumberAt( IJsonArray *iface, UINT32 index, DOUBLE *value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetNumber( impl->elements[index], value );
}

static HRESULT WINAPI json_array_GetBooleanAt( IJsonArray *iface, UINT32 index, boolean *value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetBoolean( impl->elements[index], value );
}

static const struct IJsonArrayVtbl json_array_vtbl =
{
    json_array_QueryInterface,
    json_array_AddRef,
    json_array_Release,
    /* IInspectable methods */
    json_array_GetIids,
    json_array_GetRuntimeClassName,
    json_array_GetTrustLevel,
    /* IJsonArray methods */
    json_array_GetObjectAt,
    json_array_GetArrayAt,
    json_array_GetStringAt,
    json_array_GetNumberAt,
    json_array_GetBooleanAt,
};

static HRESULT WINAPI json_array_value_QueryInterface( IJsonValue *iface, REFIID iid, void **out )
{
    struct json_array *impl = impl_from_IJsonValue( iface );
    return IJsonArray_QueryInterface( &impl->IJsonArray_iface, iid, out );
}

static ULONG WINAPI json_array_value_AddRef( IJsonValue *iface )
{
    struct json_array *impl = impl_from_IJsonValue( iface );
    return IJsonArray_AddRef( &impl->IJsonArray_iface );
}

static ULONG WINAPI json_array_value_Release( IJsonValue *iface )
{
    struct json_array *impl = impl_from_IJsonValue( iface );
    return IJsonArray_Release( &impl->IJsonArray_iface );
}

static HRESULT WINAPI json_array_value_GetIids( IJsonValue *iface, ULONG *iid_count, IID **iids )
{
    struct json_array *impl = impl_from_IJsonValue( iface );
    return IJsonArray_GetIids( &impl->IJsonArray_iface, iid_count, iids );
}

static HRESULT WINAPI json_array_value_GetRuntimeClassName( IJsonValue *iface, HSTRING *class_name )
{
    struct json_array *impl = impl_from_IJsonValue( iface );
    return IJsonArray_GetRuntimeClassName( &impl->IJsonArray_iface, class_name );
}

static HRESULT WINAPI json_array_value_GetTrustLevel( IJsonValue *iface, TrustLevel *trust_level )
{
    struct json_array *impl = impl_from_IJsonValue( iface );
    return IJsonArray_GetTrustLevel( &impl->IJsonArray_iface, trust_level );
}

static HRESULT WINAPI json_array_value_get_ValueType( IJsonValue *iface, JsonValueType *value )
{
    if (!value) return E_POINTER;
    *value = JsonValueType_Array;
    return S_OK;
}

static HRESULT WINAPI json_array_value_Stringify( IJsonValue *iface, HSTRING *value )
{
    struct json_array *impl = impl_from_IJsonValue( iface );
    UINT32 capacity = 32, length = 0, item_length, i;
    const WCHAR *item_buffer;
    HSTRING item = NULL;
    WCHAR *buffer, *new_buffer;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = NULL;
    if (!(buffer = malloc( capacity * sizeof(*buffer) ))) return E_OUTOFMEMORY;

    buffer[length++] = '[';
    for (i = 0; i < impl->length; ++i)
    {
        if (FAILED(hr = IJsonValue_Stringify( impl->elements[i], &item ))) goto done;
        item_buffer = WindowsGetStringRawBuffer( item, &item_length );

        if (item_length > UINT32_MAX - length - 2)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        if (length + item_length + 2 > capacity)
        {
            capacity = max( capacity * 3 / 2, length + item_length + 2 );
            if (!(new_buffer = realloc( buffer, capacity * sizeof(*new_buffer) )))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
            buffer = new_buffer;
        }

        if (i) buffer[length++] = ',';
        memcpy( buffer + length, item_buffer, item_length * sizeof(*buffer) );
        length += item_length;
        WindowsDeleteString( item );
        item = NULL;
    }

    buffer[length++] = ']';
    hr = WindowsCreateString( buffer, length, value );

done:
    WindowsDeleteString( item );
    free( buffer );
    return hr;
}

static HRESULT WINAPI json_array_value_GetString( IJsonValue *iface, HSTRING *value )
{
    return E_ILLEGAL_METHOD_CALL;
}

static HRESULT WINAPI json_array_value_GetNumber( IJsonValue *iface, DOUBLE *value )
{
    return E_ILLEGAL_METHOD_CALL;
}

static HRESULT WINAPI json_array_value_GetBoolean( IJsonValue *iface, boolean *value )
{
    return E_ILLEGAL_METHOD_CALL;
}

static HRESULT WINAPI json_array_value_GetArray( IJsonValue *iface, IJsonArray **value )
{
    struct json_array *impl = impl_from_IJsonValue( iface );

    if (!value) return E_POINTER;
    *value = &impl->IJsonArray_iface;
    IJsonArray_AddRef( *value );
    return S_OK;
}

static HRESULT WINAPI json_array_value_GetObject( IJsonValue *iface, IJsonObject **value )
{
    return E_ILLEGAL_METHOD_CALL;
}

static const struct IJsonValueVtbl json_array_value_vtbl =
{
    json_array_value_QueryInterface,
    json_array_value_AddRef,
    json_array_value_Release,
    /* IInspectable methods */
    json_array_value_GetIids,
    json_array_value_GetRuntimeClassName,
    json_array_value_GetTrustLevel,
    /* IJsonValue methods */
    json_array_value_get_ValueType,
    json_array_value_Stringify,
    json_array_value_GetString,
    json_array_value_GetNumber,
    json_array_value_GetBoolean,
    json_array_value_GetArray,
    json_array_value_GetObject,
};

static HRESULT WINAPI json_array_vector_QueryInterface( IVector_IJsonValue *iface,
                                                         REFIID iid, void **out )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    return IJsonArray_QueryInterface( &impl->IJsonArray_iface, iid, out );
}

static ULONG WINAPI json_array_vector_AddRef( IVector_IJsonValue *iface )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    return IJsonArray_AddRef( &impl->IJsonArray_iface );
}

static ULONG WINAPI json_array_vector_Release( IVector_IJsonValue *iface )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    return IJsonArray_Release( &impl->IJsonArray_iface );
}

static HRESULT WINAPI json_array_vector_GetIids( IVector_IJsonValue *iface,
                                                  ULONG *iid_count, IID **iids )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    return IJsonArray_GetIids( &impl->IJsonArray_iface, iid_count, iids );
}

static HRESULT WINAPI json_array_vector_GetRuntimeClassName( IVector_IJsonValue *iface,
                                                              HSTRING *class_name )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    return IJsonArray_GetRuntimeClassName( &impl->IJsonArray_iface, class_name );
}

static HRESULT WINAPI json_array_vector_GetTrustLevel( IVector_IJsonValue *iface,
                                                        TrustLevel *trust_level )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    return IJsonArray_GetTrustLevel( &impl->IJsonArray_iface, trust_level );
}

static HRESULT WINAPI json_array_vector_GetAt( IVector_IJsonValue *iface, UINT32 index,
                                                IJsonValue **value )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );

    if (!value) return E_POINTER;
    *value = NULL;
    if (index >= impl->length) return E_BOUNDS;

    *value = impl->elements[index];
    IJsonValue_AddRef( *value );
    return S_OK;
}

static HRESULT WINAPI json_array_vector_get_Size( IVector_IJsonValue *iface, UINT32 *value )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    if (!value) return E_POINTER;
    *value = impl->length;
    return S_OK;
}

static HRESULT WINAPI json_array_vector_GetView( IVector_IJsonValue *iface,
                                                  IVectorView_IJsonValue **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    if (!value) return E_POINTER;
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI json_array_vector_IndexOf( IVector_IJsonValue *iface, IJsonValue *element,
                                                  UINT32 *index, boolean *found )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    UINT32 i;

    if (!index || !found) return E_POINTER;
    for (i = 0; i < impl->length && impl->elements[i] != element; ++i);
    *found = i < impl->length;
    *index = *found ? i : 0;
    return S_OK;
}

static HRESULT WINAPI json_array_vector_SetAt( IVector_IJsonValue *iface, UINT32 index,
                                                IJsonValue *value )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );

    if (index >= impl->length) return E_BOUNDS;
    if (!value) return E_POINTER;

    IJsonValue_AddRef( value );
    IJsonValue_Release( impl->elements[index] );
    impl->elements[index] = value;
    return S_OK;
}

static HRESULT WINAPI json_array_vector_InsertAt( IVector_IJsonValue *iface, UINT32 index,
                                                   IJsonValue *value )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    IJsonValue **new_elements;

    if (index > impl->length) return E_BOUNDS;
    if (!value) return E_POINTER;

    if (impl->length == impl->capacity)
    {
        UINT32 capacity = max( 32, impl->capacity * 3 / 2 );
        if (!(new_elements = realloc( impl->elements, capacity * sizeof(*new_elements) )))
            return E_OUTOFMEMORY;
        impl->elements = new_elements;
        impl->capacity = capacity;
    }

    memmove( impl->elements + index + 1, impl->elements + index,
             (impl->length - index) * sizeof(*impl->elements) );
    impl->elements[index] = value;
    ++impl->length;
    IJsonValue_AddRef( value );
    return S_OK;
}

static HRESULT WINAPI json_array_vector_RemoveAt( IVector_IJsonValue *iface, UINT32 index )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );

    if (index >= impl->length) return E_BOUNDS;
    IJsonValue_Release( impl->elements[index] );
    --impl->length;
    memmove( impl->elements + index, impl->elements + index + 1,
             (impl->length - index) * sizeof(*impl->elements) );
    return S_OK;
}

static HRESULT WINAPI json_array_vector_Append( IVector_IJsonValue *iface, IJsonValue *value )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    return json_array_vector_InsertAt( iface, impl->length, value );
}

static HRESULT WINAPI json_array_vector_RemoveAtEnd( IVector_IJsonValue *iface )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    if (!impl->length) return E_BOUNDS;
    return json_array_vector_RemoveAt( iface, impl->length - 1 );
}

static HRESULT WINAPI json_array_vector_Clear( IVector_IJsonValue *iface )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    while (impl->length) IJsonValue_Release( impl->elements[--impl->length] );
    return S_OK;
}

static HRESULT WINAPI json_array_vector_GetMany( IVector_IJsonValue *iface, UINT32 start_index,
                                                  UINT32 items_size, IJsonValue **items,
                                                  UINT32 *count )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    UINT32 i, available;

    if (!items || !count) return E_POINTER;
    if (start_index > impl->length) return E_BOUNDS;

    available = min( items_size, impl->length - start_index );
    for (i = 0; i < available; ++i)
    {
        items[i] = impl->elements[start_index + i];
        IJsonValue_AddRef( items[i] );
    }
    *count = available;
    return S_OK;
}

static HRESULT WINAPI json_array_vector_ReplaceAll( IVector_IJsonValue *iface, UINT32 count,
                                                     IJsonValue **items )
{
    struct json_array *impl = impl_from_IVector_IJsonValue( iface );
    IJsonValue **new_elements = NULL;
    UINT32 i;

    if (count && !items) return E_POINTER;
    if (count && !(new_elements = malloc( count * sizeof(*new_elements) ))) return E_OUTOFMEMORY;
    for (i = 0; i < count; ++i)
    {
        new_elements[i] = items[i];
        IJsonValue_AddRef( new_elements[i] );
    }
    while (impl->length) IJsonValue_Release( impl->elements[--impl->length] );
    free( impl->elements );
    impl->elements = new_elements;
    impl->length = impl->capacity = count;
    return S_OK;
}

static const struct IVector_IJsonValueVtbl json_array_vector_vtbl =
{
    json_array_vector_QueryInterface,
    json_array_vector_AddRef,
    json_array_vector_Release,
    /* IInspectable methods */
    json_array_vector_GetIids,
    json_array_vector_GetRuntimeClassName,
    json_array_vector_GetTrustLevel,
    /* IVector<IJsonValue *> methods */
    json_array_vector_GetAt,
    json_array_vector_get_Size,
    json_array_vector_GetView,
    json_array_vector_IndexOf,
    json_array_vector_SetAt,
    json_array_vector_InsertAt,
    json_array_vector_RemoveAt,
    json_array_vector_Append,
    json_array_vector_RemoveAtEnd,
    json_array_vector_Clear,
    json_array_vector_GetMany,
    json_array_vector_ReplaceAll,
};

struct json_array_statics
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct json_array_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct json_array_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct json_array_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct json_array_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct json_array_statics *impl = impl_from_IActivationFactory( iface );
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
    struct json_array *impl;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    *instance = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IJsonArray_iface.lpVtbl = &json_array_vtbl;
    impl->IJsonValue_iface.lpVtbl = &json_array_value_vtbl;
    impl->IVector_IJsonValue_iface.lpVtbl = &json_array_vector_vtbl;
    impl->ref = 1;

    *instance = (IInspectable *)&impl->IJsonArray_iface;
    return S_OK;
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

static struct json_array_statics json_array_statics =
{
    {&factory_vtbl},
    1,
};

IActivationFactory *json_array_factory = &json_array_statics.IActivationFactory_iface;
