/* WinRT Windows.Web.Http.HttpClient implementation
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

struct http_client
{
    IHttpClient IHttpClient_iface;
    IClosable IClosable_iface;
    IStringable IStringable_iface;
    LONG ref;
    IHttpFilter *filter;
};

static inline struct http_client *impl_from_IHttpClient(IHttpClient *iface)
{
    return CONTAINING_RECORD(iface, struct http_client, IHttpClient_iface);
}

static inline struct http_client *impl_from_client_IClosable(IClosable *iface)
{
    return CONTAINING_RECORD(iface, struct http_client, IClosable_iface);
}

static inline struct http_client *impl_from_IStringable(IStringable *iface)
{
    return CONTAINING_RECORD(iface, struct http_client, IStringable_iface);
}

static ULONG http_client_addref(struct http_client *impl)
{
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("impl %p increasing refcount to %lu.\n", impl, ref);
    return ref;
}

static ULONG http_client_release(struct http_client *impl)
{
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("impl %p decreasing refcount to %lu.\n", impl, ref);
    if (!ref)
    {
        if (impl->filter) IHttpFilter_Release(impl->filter);
        free(impl);
    }
    return ref;
}

static HRESULT http_client_query_interface(struct http_client *impl, REFIID iid, void **out)
{
    if (!out) return E_POINTER;

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) || IsEqualGUID(iid, &IID_IHttpClient))
        *out = &impl->IHttpClient_iface;
    else if (IsEqualGUID(iid, &IID_IClosable))
        *out = &impl->IClosable_iface;
    else if (IsEqualGUID(iid, &IID_IStringable))
        *out = &impl->IStringable_iface;
    else
    {
        FIXME("interface %s not implemented.\n", debugstr_guid(iid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    http_client_addref(impl);
    return S_OK;
}

static HRESULT WINAPI client_QueryInterface(IHttpClient *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);
    return http_client_query_interface(impl_from_IHttpClient(iface), iid, out);
}

static ULONG WINAPI client_AddRef(IHttpClient *iface)
{
    return http_client_addref(impl_from_IHttpClient(iface));
}

static ULONG WINAPI client_Release(IHttpClient *iface)
{
    return http_client_release(impl_from_IHttpClient(iface));
}

static HRESULT WINAPI client_GetIids(IHttpClient *iface, ULONG *count, IID **iids)
{
    IID *result;

    TRACE("iface %p, count %p, iids %p.\n", iface, count, iids);
    if (!count || !iids) return E_POINTER;
    if (!(result = CoTaskMemAlloc(3 * sizeof(*result)))) return E_OUTOFMEMORY;
    result[0] = IID_IHttpClient;
    result[1] = IID_IClosable;
    result[2] = IID_IStringable;
    *count = 3;
    *iids = result;
    return S_OK;
}

static HRESULT WINAPI client_GetRuntimeClassName(IHttpClient *iface, HSTRING *class_name)
{
    TRACE("iface %p, class_name %p.\n", iface, class_name);
    if (!class_name) return E_POINTER;
    return WindowsCreateString(RuntimeClass_Windows_Web_Http_HttpClient,
                               ARRAY_SIZE(RuntimeClass_Windows_Web_Http_HttpClient) - 1, class_name);
}

static HRESULT WINAPI client_GetTrustLevel(IHttpClient *iface, TrustLevel *trust_level)
{
    if (!trust_level) return E_POINTER;
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT async_not_implemented(IInspectable **operation)
{
    if (!operation) return E_POINTER;
    *operation = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI client_DeleteAsync(IHttpClient *iface, IInspectable *uri, IInspectable **operation)
{
    FIXME("iface %p, uri %p, operation %p stub.\n", iface, uri, operation);
    return async_not_implemented(operation);
}

static HRESULT WINAPI client_GetAsync(IHttpClient *iface, IInspectable *uri, IInspectable **operation)
{
    FIXME("iface %p, uri %p, operation %p stub.\n", iface, uri, operation);
    return async_not_implemented(operation);
}

static HRESULT WINAPI client_GetWithOptionAsync(IHttpClient *iface, IInspectable *uri,
                                                 HttpCompletionOption option, IInspectable **operation)
{
    FIXME("iface %p, uri %p, option %d, operation %p stub.\n", iface, uri, option, operation);
    return async_not_implemented(operation);
}

static HRESULT WINAPI client_GetBufferAsync(IHttpClient *iface, IInspectable *uri, IInspectable **operation)
{
    FIXME("iface %p, uri %p, operation %p stub.\n", iface, uri, operation);
    return async_not_implemented(operation);
}

static HRESULT WINAPI client_GetInputStreamAsync(IHttpClient *iface, IInspectable *uri, IInspectable **operation)
{
    FIXME("iface %p, uri %p, operation %p stub.\n", iface, uri, operation);
    return async_not_implemented(operation);
}

static HRESULT WINAPI client_GetStringAsync(IHttpClient *iface, IInspectable *uri, IInspectable **operation)
{
    FIXME("iface %p, uri %p, operation %p stub.\n", iface, uri, operation);
    return async_not_implemented(operation);
}

static HRESULT WINAPI client_PostAsync(IHttpClient *iface, IInspectable *uri, IInspectable *content,
                                       IInspectable **operation)
{
    FIXME("iface %p, uri %p, content %p, operation %p stub.\n", iface, uri, content, operation);
    return async_not_implemented(operation);
}

static HRESULT WINAPI client_PutAsync(IHttpClient *iface, IInspectable *uri, IInspectable *content,
                                      IInspectable **operation)
{
    FIXME("iface %p, uri %p, content %p, operation %p stub.\n", iface, uri, content, operation);
    return async_not_implemented(operation);
}

static HRESULT WINAPI client_SendRequestAsync(IHttpClient *iface, IInspectable *request, IInspectable **operation)
{
    struct http_client *impl = impl_from_IHttpClient(iface);

    TRACE("iface %p, request %p, operation %p.\n", iface, request, operation);
    if (!operation) return E_POINTER;
    *operation = NULL;
    if (!impl->filter) return E_NOTIMPL;
    return IHttpFilter_SendRequestAsync(impl->filter, request, operation);
}

static HRESULT WINAPI client_SendRequestWithOptionAsync(IHttpClient *iface, IInspectable *request,
                                                         HttpCompletionOption option, IInspectable **operation)
{
    FIXME("iface %p, request %p, option %d, operation %p stub.\n", iface, request, option, operation);
    return client_SendRequestAsync(iface, request, operation);
}

static HRESULT WINAPI client_get_DefaultRequestHeaders(IHttpClient *iface, IInspectable **value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    if (!value) return E_POINTER;
    *value = NULL;
    return E_NOTIMPL;
}

static const IHttpClientVtbl http_client_vtbl =
{
    client_QueryInterface,
    client_AddRef,
    client_Release,
    client_GetIids,
    client_GetRuntimeClassName,
    client_GetTrustLevel,
    client_DeleteAsync,
    client_GetAsync,
    client_GetWithOptionAsync,
    client_GetBufferAsync,
    client_GetInputStreamAsync,
    client_GetStringAsync,
    client_PostAsync,
    client_PutAsync,
    client_SendRequestAsync,
    client_SendRequestWithOptionAsync,
    client_get_DefaultRequestHeaders,
};

static HRESULT WINAPI client_closable_QueryInterface(IClosable *iface, REFIID iid, void **out)
{
    return http_client_query_interface(impl_from_client_IClosable(iface), iid, out);
}

static ULONG WINAPI client_closable_AddRef(IClosable *iface)
{
    return http_client_addref(impl_from_client_IClosable(iface));
}

static ULONG WINAPI client_closable_Release(IClosable *iface)
{
    return http_client_release(impl_from_client_IClosable(iface));
}

static HRESULT WINAPI client_closable_GetIids(IClosable *iface, ULONG *count, IID **iids)
{
    struct http_client *impl = impl_from_client_IClosable(iface);
    return client_GetIids(&impl->IHttpClient_iface, count, iids);
}

static HRESULT WINAPI client_closable_GetRuntimeClassName(IClosable *iface, HSTRING *class_name)
{
    struct http_client *impl = impl_from_client_IClosable(iface);
    return client_GetRuntimeClassName(&impl->IHttpClient_iface, class_name);
}

static HRESULT WINAPI client_closable_GetTrustLevel(IClosable *iface, TrustLevel *trust_level)
{
    struct http_client *impl = impl_from_client_IClosable(iface);
    return client_GetTrustLevel(&impl->IHttpClient_iface, trust_level);
}

static HRESULT WINAPI client_closable_Close(IClosable *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static const IClosableVtbl client_closable_vtbl =
{
    client_closable_QueryInterface,
    client_closable_AddRef,
    client_closable_Release,
    client_closable_GetIids,
    client_closable_GetRuntimeClassName,
    client_closable_GetTrustLevel,
    client_closable_Close,
};

static HRESULT WINAPI stringable_QueryInterface(IStringable *iface, REFIID iid, void **out)
{
    return http_client_query_interface(impl_from_IStringable(iface), iid, out);
}

static ULONG WINAPI stringable_AddRef(IStringable *iface)
{
    return http_client_addref(impl_from_IStringable(iface));
}

static ULONG WINAPI stringable_Release(IStringable *iface)
{
    return http_client_release(impl_from_IStringable(iface));
}

static HRESULT WINAPI stringable_GetIids(IStringable *iface, ULONG *count, IID **iids)
{
    struct http_client *impl = impl_from_IStringable(iface);
    return client_GetIids(&impl->IHttpClient_iface, count, iids);
}

static HRESULT WINAPI stringable_GetRuntimeClassName(IStringable *iface, HSTRING *class_name)
{
    struct http_client *impl = impl_from_IStringable(iface);
    return client_GetRuntimeClassName(&impl->IHttpClient_iface, class_name);
}

static HRESULT WINAPI stringable_GetTrustLevel(IStringable *iface, TrustLevel *trust_level)
{
    struct http_client *impl = impl_from_IStringable(iface);
    return client_GetTrustLevel(&impl->IHttpClient_iface, trust_level);
}

static HRESULT WINAPI stringable_ToString(IStringable *iface, HSTRING *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    return WindowsCreateString(L"Windows.Web.Http.HttpClient", 27, value);
}

static const IStringableVtbl stringable_vtbl =
{
    stringable_QueryInterface,
    stringable_AddRef,
    stringable_Release,
    stringable_GetIids,
    stringable_GetRuntimeClassName,
    stringable_GetTrustLevel,
    stringable_ToString,
};

static HRESULT http_client_create(IHttpFilter *filter, IHttpClient **client)
{
    struct http_client *impl;

    if (!client) return E_POINTER;
    *client = NULL;
    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;

    impl->IHttpClient_iface.lpVtbl = &http_client_vtbl;
    impl->IClosable_iface.lpVtbl = &client_closable_vtbl;
    impl->IStringable_iface.lpVtbl = &stringable_vtbl;
    impl->ref = 1;
    impl->filter = filter;
    if (filter) IHttpFilter_AddRef(filter);
    *client = &impl->IHttpClient_iface;
    TRACE("created client %p with filter %p.\n", *client, filter);
    return S_OK;
}

struct http_client_activation_factory
{
    IActivationFactory IActivationFactory_iface;
    IHttpClientFactory IHttpClientFactory_iface;
    LONG ref;
};

static inline struct http_client_activation_factory *impl_from_client_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct http_client_activation_factory, IActivationFactory_iface);
}

static inline struct http_client_activation_factory *impl_from_IHttpClientFactory(IHttpClientFactory *iface)
{
    return CONTAINING_RECORD(iface, struct http_client_activation_factory, IHttpClientFactory_iface);
}

static HRESULT client_factory_query_interface(struct http_client_activation_factory *impl, REFIID iid, void **out)
{
    if (!out) return E_POINTER;
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) || IsEqualGUID(iid, &IID_IActivationFactory))
        *out = &impl->IActivationFactory_iface;
    else if (IsEqualGUID(iid, &IID_IHttpClientFactory))
        *out = &impl->IHttpClientFactory_iface;
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }
    InterlockedIncrement(&impl->ref);
    return S_OK;
}

static HRESULT WINAPI client_factory_QueryInterface(IActivationFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);
    return client_factory_query_interface(impl_from_client_IActivationFactory(iface), iid, out);
}

static ULONG WINAPI client_factory_AddRef(IActivationFactory *iface)
{
    return InterlockedIncrement(&impl_from_client_IActivationFactory(iface)->ref);
}

static ULONG WINAPI client_factory_Release(IActivationFactory *iface)
{
    return InterlockedDecrement(&impl_from_client_IActivationFactory(iface)->ref);
}

static HRESULT WINAPI client_factory_GetIids(IActivationFactory *iface, ULONG *count, IID **iids)
{
    FIXME("iface %p, count %p, iids %p stub.\n", iface, count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI client_factory_GetRuntimeClassName(IActivationFactory *iface, HSTRING *class_name)
{
    if (!class_name) return E_POINTER;
    return WindowsCreateString(RuntimeClass_Windows_Web_Http_HttpClient,
                               ARRAY_SIZE(RuntimeClass_Windows_Web_Http_HttpClient) - 1, class_name);
}

static HRESULT WINAPI client_factory_GetTrustLevel(IActivationFactory *iface, TrustLevel *trust_level)
{
    if (!trust_level) return E_POINTER;
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI client_factory_ActivateInstance(IActivationFactory *iface, IInspectable **instance)
{
    return http_client_create(NULL, (IHttpClient **)instance);
}

static const IActivationFactoryVtbl client_activation_factory_vtbl =
{
    client_factory_QueryInterface,
    client_factory_AddRef,
    client_factory_Release,
    client_factory_GetIids,
    client_factory_GetRuntimeClassName,
    client_factory_GetTrustLevel,
    client_factory_ActivateInstance,
};

static HRESULT WINAPI client_specific_factory_QueryInterface(IHttpClientFactory *iface, REFIID iid, void **out)
{
    return client_factory_query_interface(impl_from_IHttpClientFactory(iface), iid, out);
}

static ULONG WINAPI client_specific_factory_AddRef(IHttpClientFactory *iface)
{
    return InterlockedIncrement(&impl_from_IHttpClientFactory(iface)->ref);
}

static ULONG WINAPI client_specific_factory_Release(IHttpClientFactory *iface)
{
    return InterlockedDecrement(&impl_from_IHttpClientFactory(iface)->ref);
}

static HRESULT WINAPI client_specific_factory_GetIids(IHttpClientFactory *iface, ULONG *count, IID **iids)
{
    FIXME("iface %p, count %p, iids %p stub.\n", iface, count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI client_specific_factory_GetRuntimeClassName(IHttpClientFactory *iface, HSTRING *class_name)
{
    if (!class_name) return E_POINTER;
    return WindowsCreateString(RuntimeClass_Windows_Web_Http_HttpClient,
                               ARRAY_SIZE(RuntimeClass_Windows_Web_Http_HttpClient) - 1, class_name);
}

static HRESULT WINAPI client_specific_factory_GetTrustLevel(IHttpClientFactory *iface, TrustLevel *trust_level)
{
    if (!trust_level) return E_POINTER;
    *trust_level = BaseTrust;
    return S_OK;
}

static HRESULT WINAPI client_specific_factory_Create(IHttpClientFactory *iface, IHttpFilter *filter,
                                                      IHttpClient **client)
{
    TRACE("iface %p, filter %p, client %p.\n", iface, filter, client);
    if (!filter) return E_INVALIDARG;
    return http_client_create(filter, client);
}

static const IHttpClientFactoryVtbl client_specific_factory_vtbl =
{
    client_specific_factory_QueryInterface,
    client_specific_factory_AddRef,
    client_specific_factory_Release,
    client_specific_factory_GetIids,
    client_specific_factory_GetRuntimeClassName,
    client_specific_factory_GetTrustLevel,
    client_specific_factory_Create,
};

static struct http_client_activation_factory http_client_factory_impl =
{
    {&client_activation_factory_vtbl},
    {&client_specific_factory_vtbl},
    1,
};

IActivationFactory *http_client_factory = &http_client_factory_impl.IActivationFactory_iface;
