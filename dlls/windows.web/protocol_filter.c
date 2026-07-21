/* WinRT Windows.Web.Http implementation
 *
 * Copyright 2026 Wine contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(web);

struct protocol_filter
{
    IHttpBaseProtocolFilter IHttpBaseProtocolFilter_iface;
    IHttpFilter IHttpFilter_iface;
    IClosable IClosable_iface;
    IHttpCacheControl IHttpCacheControl_iface;
    LONG ref;
    boolean allow_auto_redirect;
    boolean allow_ui;
    boolean automatic_decompression;
    boolean use_proxy;
    UINT32 max_connections_per_server;
    HttpCacheReadBehavior cache_read_behavior;
    HttpCacheWriteBehavior cache_write_behavior;
    IInspectable *client_certificate;
    IInspectable *proxy_credential;
    IInspectable *server_credential;
};

static inline struct protocol_filter *impl_from_IHttpBaseProtocolFilter(IHttpBaseProtocolFilter *iface)
{
    return CONTAINING_RECORD(iface, struct protocol_filter, IHttpBaseProtocolFilter_iface);
}

static inline struct protocol_filter *impl_from_IHttpFilter(IHttpFilter *iface)
{
    return CONTAINING_RECORD(iface, struct protocol_filter, IHttpFilter_iface);
}

static inline struct protocol_filter *impl_from_IClosable(IClosable *iface)
{
    return CONTAINING_RECORD(iface, struct protocol_filter, IClosable_iface);
}

static inline struct protocol_filter *impl_from_IHttpCacheControl(IHttpCacheControl *iface)
{
    return CONTAINING_RECORD(iface, struct protocol_filter, IHttpCacheControl_iface);
}

static ULONG protocol_filter_addref(struct protocol_filter *impl)
{
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("impl %p increasing refcount to %lu.\n", impl, ref);
    return ref;
}

static ULONG protocol_filter_release(struct protocol_filter *impl)
{
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("impl %p decreasing refcount to %lu.\n", impl, ref);
    if (!ref)
    {
        if (impl->client_certificate) IInspectable_Release(impl->client_certificate);
        if (impl->proxy_credential) IInspectable_Release(impl->proxy_credential);
        if (impl->server_credential) IInspectable_Release(impl->server_credential);
        free(impl);
    }
    return ref;
}

static HRESULT protocol_filter_query_interface(struct protocol_filter *impl, REFIID iid, void **out)
{
    if (!out) return E_POINTER;

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) || IsEqualGUID(iid, &IID_IHttpBaseProtocolFilter))
        *out = &impl->IHttpBaseProtocolFilter_iface;
    else if (IsEqualGUID(iid, &IID_IHttpFilter))
        *out = &impl->IHttpFilter_iface;
    else if (IsEqualGUID(iid, &IID_IClosable))
        *out = &impl->IClosable_iface;
    else if (IsEqualGUID(iid, &IID_IHttpCacheControl))
        *out = &impl->IHttpCacheControl_iface;
    else
    {
        FIXME("interface %s not implemented.\n", debugstr_guid(iid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    protocol_filter_addref(impl);
    return S_OK;
}

static HRESULT WINAPI base_QueryInterface(IHttpBaseProtocolFilter *iface, REFIID iid, void **out)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);
    return protocol_filter_query_interface(impl, iid, out);
}

static ULONG WINAPI base_AddRef(IHttpBaseProtocolFilter *iface)
{
    return protocol_filter_addref(impl_from_IHttpBaseProtocolFilter(iface));
}

static ULONG WINAPI base_Release(IHttpBaseProtocolFilter *iface)
{
    return protocol_filter_release(impl_from_IHttpBaseProtocolFilter(iface));
}

static HRESULT WINAPI base_GetIids(IHttpBaseProtocolFilter *iface, ULONG *iid_count, IID **iids)
{
    IID *result;

    TRACE("iface %p, iid_count %p, iids %p.\n", iface, iid_count, iids);
    if (!iid_count || !iids) return E_POINTER;
    if (!(result = CoTaskMemAlloc(3 * sizeof(*result)))) return E_OUTOFMEMORY;
    result[0] = IID_IHttpBaseProtocolFilter;
    result[1] = IID_IHttpFilter;
    result[2] = IID_IClosable;
    *iid_count = 3;
    *iids = result;
    return S_OK;
}

static HRESULT WINAPI base_GetRuntimeClassName(IHttpBaseProtocolFilter *iface, HSTRING *class_name)
{
    TRACE("iface %p, class_name %p.\n", iface, class_name);
    if (!class_name) return E_POINTER;
    return WindowsCreateString(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter,
                               ARRAY_SIZE(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter) - 1,
                               class_name);
}

static HRESULT WINAPI base_GetTrustLevel(IHttpBaseProtocolFilter *iface, TrustLevel *trust_level)
{
    TRACE("iface %p, trust_level %p.\n", iface, trust_level);
    if (!trust_level) return E_POINTER;
    *trust_level = BaseTrust;
    return S_OK;
}

#define DEFINE_BOOL_PROPERTY(name, member) \
static HRESULT WINAPI base_get_##name(IHttpBaseProtocolFilter *iface, boolean *value) \
{ \
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface); \
    TRACE("iface %p, value %p.\n", iface, value); \
    if (!value) return E_POINTER; \
    *value = impl->member; \
    return S_OK; \
} \
static HRESULT WINAPI base_put_##name(IHttpBaseProtocolFilter *iface, boolean value) \
{ \
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface); \
    TRACE("iface %p, value %u.\n", iface, value); \
    impl->member = !!value; \
    return S_OK; \
}

DEFINE_BOOL_PROPERTY(AllowAutoRedirect, allow_auto_redirect)
DEFINE_BOOL_PROPERTY(AllowUI, allow_ui)
DEFINE_BOOL_PROPERTY(AutomaticDecompression, automatic_decompression)
DEFINE_BOOL_PROPERTY(UseProxy, use_proxy)

static HRESULT WINAPI base_get_CacheControl(IHttpBaseProtocolFilter *iface, IHttpCacheControl **value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = &impl->IHttpCacheControl_iface;
    protocol_filter_addref(impl);
    return S_OK;
}

static HRESULT WINAPI base_get_CookieManager(IHttpBaseProtocolFilter *iface, IInspectable **value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    if (!value) return E_POINTER;
    *value = NULL;
    return S_OK;
}

static HRESULT get_object_property(IInspectable *value, IInspectable **out)
{
    if (!out) return E_POINTER;
    *out = value;
    if (value) IInspectable_AddRef(value);
    return S_OK;
}

static HRESULT set_object_property(IInspectable **property, IInspectable *value)
{
    if (value) IInspectable_AddRef(value);
    if (*property) IInspectable_Release(*property);
    *property = value;
    return S_OK;
}

static HRESULT WINAPI base_get_ClientCertificate(IHttpBaseProtocolFilter *iface, IInspectable **value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    return get_object_property(impl->client_certificate, value);
}

static HRESULT WINAPI base_put_ClientCertificate(IHttpBaseProtocolFilter *iface, IInspectable *value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    return set_object_property(&impl->client_certificate, value);
}

static HRESULT WINAPI base_get_IgnorableServerCertificateErrors(IHttpBaseProtocolFilter *iface, IInspectable **value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    if (!value) return E_POINTER;
    *value = NULL;
    return S_OK;
}

static HRESULT WINAPI base_get_MaxConnectionsPerServer(IHttpBaseProtocolFilter *iface, UINT32 *value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = impl->max_connections_per_server;
    return S_OK;
}

static HRESULT WINAPI base_put_MaxConnectionsPerServer(IHttpBaseProtocolFilter *iface, UINT32 value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %u.\n", iface, value);
    if (!value) return E_INVALIDARG;
    impl->max_connections_per_server = value;
    return S_OK;
}

static HRESULT WINAPI base_get_ProxyCredential(IHttpBaseProtocolFilter *iface, IInspectable **value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    return get_object_property(impl->proxy_credential, value);
}

static HRESULT WINAPI base_put_ProxyCredential(IHttpBaseProtocolFilter *iface, IInspectable *value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    return set_object_property(&impl->proxy_credential, value);
}

static HRESULT WINAPI base_get_ServerCredential(IHttpBaseProtocolFilter *iface, IInspectable **value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    return get_object_property(impl->server_credential, value);
}

static HRESULT WINAPI base_put_ServerCredential(IHttpBaseProtocolFilter *iface, IInspectable *value)
{
    struct protocol_filter *impl = impl_from_IHttpBaseProtocolFilter(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    return set_object_property(&impl->server_credential, value);
}

static const IHttpBaseProtocolFilterVtbl protocol_filter_vtbl =
{
    base_QueryInterface,
    base_AddRef,
    base_Release,
    base_GetIids,
    base_GetRuntimeClassName,
    base_GetTrustLevel,
    base_get_AllowAutoRedirect,
    base_put_AllowAutoRedirect,
    base_get_AllowUI,
    base_put_AllowUI,
    base_get_AutomaticDecompression,
    base_put_AutomaticDecompression,
    base_get_CacheControl,
    base_get_CookieManager,
    base_get_ClientCertificate,
    base_put_ClientCertificate,
    base_get_IgnorableServerCertificateErrors,
    base_get_MaxConnectionsPerServer,
    base_put_MaxConnectionsPerServer,
    base_get_ProxyCredential,
    base_put_ProxyCredential,
    base_get_ServerCredential,
    base_put_ServerCredential,
    base_get_UseProxy,
    base_put_UseProxy,
};

static HRESULT WINAPI filter_QueryInterface(IHttpFilter *iface, REFIID iid, void **out)
{
    return protocol_filter_query_interface(impl_from_IHttpFilter(iface), iid, out);
}

static ULONG WINAPI filter_AddRef(IHttpFilter *iface)
{
    return protocol_filter_addref(impl_from_IHttpFilter(iface));
}

static ULONG WINAPI filter_Release(IHttpFilter *iface)
{
    return protocol_filter_release(impl_from_IHttpFilter(iface));
}

static HRESULT WINAPI filter_GetIids(IHttpFilter *iface, ULONG *count, IID **iids)
{
    struct protocol_filter *impl = impl_from_IHttpFilter(iface);
    return base_GetIids(&impl->IHttpBaseProtocolFilter_iface, count, iids);
}

static HRESULT WINAPI filter_GetRuntimeClassName(IHttpFilter *iface, HSTRING *class_name)
{
    struct protocol_filter *impl = impl_from_IHttpFilter(iface);
    return base_GetRuntimeClassName(&impl->IHttpBaseProtocolFilter_iface, class_name);
}

static HRESULT WINAPI filter_GetTrustLevel(IHttpFilter *iface, TrustLevel *trust_level)
{
    struct protocol_filter *impl = impl_from_IHttpFilter(iface);
    return base_GetTrustLevel(&impl->IHttpBaseProtocolFilter_iface, trust_level);
}

static HRESULT WINAPI filter_SendRequestAsync(IHttpFilter *iface, IInspectable *request, IInspectable **operation)
{
    FIXME("iface %p, request %p, operation %p stub.\n", iface, request, operation);
    if (!operation) return E_POINTER;
    *operation = NULL;
    return E_NOTIMPL;
}

static const IHttpFilterVtbl http_filter_vtbl =
{
    filter_QueryInterface,
    filter_AddRef,
    filter_Release,
    filter_GetIids,
    filter_GetRuntimeClassName,
    filter_GetTrustLevel,
    filter_SendRequestAsync,
};

static HRESULT WINAPI closable_QueryInterface(IClosable *iface, REFIID iid, void **out)
{
    return protocol_filter_query_interface(impl_from_IClosable(iface), iid, out);
}

static ULONG WINAPI closable_AddRef(IClosable *iface)
{
    return protocol_filter_addref(impl_from_IClosable(iface));
}

static ULONG WINAPI closable_Release(IClosable *iface)
{
    return protocol_filter_release(impl_from_IClosable(iface));
}

static HRESULT WINAPI closable_GetIids(IClosable *iface, ULONG *count, IID **iids)
{
    struct protocol_filter *impl = impl_from_IClosable(iface);
    return base_GetIids(&impl->IHttpBaseProtocolFilter_iface, count, iids);
}

static HRESULT WINAPI closable_GetRuntimeClassName(IClosable *iface, HSTRING *class_name)
{
    struct protocol_filter *impl = impl_from_IClosable(iface);
    return base_GetRuntimeClassName(&impl->IHttpBaseProtocolFilter_iface, class_name);
}

static HRESULT WINAPI closable_GetTrustLevel(IClosable *iface, TrustLevel *trust_level)
{
    struct protocol_filter *impl = impl_from_IClosable(iface);
    return base_GetTrustLevel(&impl->IHttpBaseProtocolFilter_iface, trust_level);
}

static HRESULT WINAPI closable_Close(IClosable *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static const IClosableVtbl closable_vtbl =
{
    closable_QueryInterface,
    closable_AddRef,
    closable_Release,
    closable_GetIids,
    closable_GetRuntimeClassName,
    closable_GetTrustLevel,
    closable_Close,
};

static HRESULT WINAPI cache_QueryInterface(IHttpCacheControl *iface, REFIID iid, void **out)
{
    return protocol_filter_query_interface(impl_from_IHttpCacheControl(iface), iid, out);
}

static ULONG WINAPI cache_AddRef(IHttpCacheControl *iface)
{
    return protocol_filter_addref(impl_from_IHttpCacheControl(iface));
}

static ULONG WINAPI cache_Release(IHttpCacheControl *iface)
{
    return protocol_filter_release(impl_from_IHttpCacheControl(iface));
}

static HRESULT WINAPI cache_GetIids(IHttpCacheControl *iface, ULONG *count, IID **iids)
{
    IID *result;
    if (!count || !iids) return E_POINTER;
    if (!(result = CoTaskMemAlloc(sizeof(*result)))) return E_OUTOFMEMORY;
    result[0] = IID_IHttpCacheControl;
    *count = 1;
    *iids = result;
    return S_OK;
}

static HRESULT WINAPI cache_GetRuntimeClassName(IHttpCacheControl *iface, HSTRING *class_name)
{
    TRACE("iface %p, class_name %p.\n", iface, class_name);
    if (!class_name) return E_POINTER;
    return WindowsCreateString(RuntimeClass_Windows_Web_Http_Filters_HttpCacheControl,
                               ARRAY_SIZE(RuntimeClass_Windows_Web_Http_Filters_HttpCacheControl) - 1,
                               class_name);
}

static HRESULT WINAPI cache_GetTrustLevel(IHttpCacheControl *iface, TrustLevel *trust_level)
{
    if (!trust_level) return E_POINTER;
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI cache_get_ReadBehavior(IHttpCacheControl *iface, HttpCacheReadBehavior *value)
{
    struct protocol_filter *impl = impl_from_IHttpCacheControl(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = impl->cache_read_behavior;
    return S_OK;
}

static HRESULT WINAPI cache_put_ReadBehavior(IHttpCacheControl *iface, HttpCacheReadBehavior value)
{
    struct protocol_filter *impl = impl_from_IHttpCacheControl(iface);
    TRACE("iface %p, value %d.\n", iface, value);
    if (value < HttpCacheReadBehavior_Default || value > HttpCacheReadBehavior_NoCache) return E_INVALIDARG;
    impl->cache_read_behavior = value;
    return S_OK;
}

static HRESULT WINAPI cache_get_WriteBehavior(IHttpCacheControl *iface, HttpCacheWriteBehavior *value)
{
    struct protocol_filter *impl = impl_from_IHttpCacheControl(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = impl->cache_write_behavior;
    return S_OK;
}

static HRESULT WINAPI cache_put_WriteBehavior(IHttpCacheControl *iface, HttpCacheWriteBehavior value)
{
    struct protocol_filter *impl = impl_from_IHttpCacheControl(iface);
    TRACE("iface %p, value %d.\n", iface, value);
    if (value < HttpCacheWriteBehavior_Default || value > HttpCacheWriteBehavior_NoCache) return E_INVALIDARG;
    impl->cache_write_behavior = value;
    return S_OK;
}

static const IHttpCacheControlVtbl cache_control_vtbl =
{
    cache_QueryInterface,
    cache_AddRef,
    cache_Release,
    cache_GetIids,
    cache_GetRuntimeClassName,
    cache_GetTrustLevel,
    cache_get_ReadBehavior,
    cache_put_ReadBehavior,
    cache_get_WriteBehavior,
    cache_put_WriteBehavior,
};

static HRESULT protocol_filter_create(IInspectable **instance)
{
    struct protocol_filter *impl;

    if (!instance) return E_POINTER;
    *instance = NULL;
    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;

    impl->IHttpBaseProtocolFilter_iface.lpVtbl = &protocol_filter_vtbl;
    impl->IHttpFilter_iface.lpVtbl = &http_filter_vtbl;
    impl->IClosable_iface.lpVtbl = &closable_vtbl;
    impl->IHttpCacheControl_iface.lpVtbl = &cache_control_vtbl;
    impl->ref = 1;
    impl->allow_auto_redirect = TRUE;
    impl->allow_ui = TRUE;
    impl->automatic_decompression = TRUE;
    impl->use_proxy = TRUE;
    impl->max_connections_per_server = 6;
    impl->cache_read_behavior = HttpCacheReadBehavior_Default;
    impl->cache_write_behavior = HttpCacheWriteBehavior_Default;

    *instance = (IInspectable *)&impl->IHttpBaseProtocolFilter_iface;
    TRACE("created instance %p.\n", *instance);
    return S_OK;
}

struct activation_factory
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct activation_factory *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct activation_factory, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IActivationFactory *iface, REFIID iid, void **out)
{
    struct activation_factory *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);
    if (!out) return E_POINTER;
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        *out = &impl->IActivationFactory_iface;
        IActivationFactory_AddRef(&impl->IActivationFactory_iface);
        return S_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    return InterlockedIncrement(&impl_from_IActivationFactory(iface)->ref);
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    return InterlockedDecrement(&impl_from_IActivationFactory(iface)->ref);
}

static HRESULT WINAPI factory_GetIids(IActivationFactory *iface, ULONG *count, IID **iids)
{
    FIXME("iface %p, count %p, iids %p stub.\n", iface, count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName(IActivationFactory *iface, HSTRING *class_name)
{
    TRACE("iface %p, class_name %p.\n", iface, class_name);
    if (!class_name) return E_POINTER;
    return WindowsCreateString(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter,
                               ARRAY_SIZE(RuntimeClass_Windows_Web_Http_Filters_HttpBaseProtocolFilter) - 1,
                               class_name);
}

static HRESULT WINAPI factory_GetTrustLevel(IActivationFactory *iface, TrustLevel *trust_level)
{
    if (!trust_level) return E_POINTER;
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI factory_ActivateInstance(IActivationFactory *iface, IInspectable **instance)
{
    TRACE("iface %p, instance %p.\n", iface, instance);
    return protocol_filter_create(instance);
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

static struct activation_factory protocol_filter_factory_impl =
{
    {&factory_vtbl},
    1,
};

IActivationFactory *protocol_filter_factory = &protocol_filter_factory_impl.IActivationFactory_iface;
