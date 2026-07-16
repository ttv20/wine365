/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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

#include "d2d1_private.h"
#include "initguid.h"
#include "wincodec.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

#define D2D_WIC_POOL_LIMIT 32

static SRWLOCK d2d_wic_pool_lock = SRWLOCK_INIT;
static struct d2d_wic_render_target *d2d_wic_pool;
static unsigned int d2d_wic_pool_count;

static BOOL d2d_wic_pool_enabled(void)
{
    static int enabled = -1;
    WCHAR path[MAX_PATH], *name;
    char value[2];

    if (enabled != -1)
        return enabled;

    if (GetEnvironmentVariableA("WINE_D2D_SMALL_WIC_POOL", value, ARRAY_SIZE(value)))
        return enabled = value[0] != '0';

    if (!GetModuleFileNameW(NULL, path, ARRAY_SIZE(path)))
        return enabled = FALSE;
    name = wcsrchr(path, '\\');
    name = name ? name + 1 : path;
    enabled = !wcsicmp(name, L"WINWORD.EXE") || !wcsicmp(name, L"EXCEL.EXE")
            || !wcsicmp(name, L"POWERPNT.EXE") || !wcsicmp(name, L"OUTLOOK.EXE");
    return enabled;
}

static inline struct d2d_wic_render_target *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_wic_render_target, IUnknown_iface);
}

static void d2d_wic_render_target_discard_cpu_glyphs(struct d2d_wic_render_target *render_target)
{
    struct d2d_wic_cpu_glyph *glyph, *next;

    for (glyph = render_target->cpu_glyphs; glyph; glyph = next)
    {
        next = glyph->next;
        free(glyph);
    }
    render_target->cpu_glyphs = NULL;
    render_target->cpu_glyph_tail = &render_target->cpu_glyphs;
}

static BOOL d2d_wic_render_target_queue_cpu_glyph(IUnknown *outer_unknown, const RECT *bounds,
        const BYTE *values, unsigned int pitch, const D2D1_COLOR_F *colour)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(outer_unknown);
    struct d2d_wic_cpu_glyph *glyph;
    unsigned int height = bounds->bottom - bounds->top;
    size_t size = (size_t)pitch * height;

    if (!d2d_wic_pool_enabled() || render_target->width > 32 || render_target->height > 32
            || !(glyph = malloc(sizeof(*glyph) + size)))
        return FALSE;

    glyph->next = NULL;
    glyph->bounds = *bounds;
    glyph->colour = *colour;
    glyph->pitch = pitch;
    memcpy(glyph->values, values, size);
    if (!render_target->cpu_glyph_tail)
        render_target->cpu_glyph_tail = &render_target->cpu_glyphs;
    *render_target->cpu_glyph_tail = glyph;
    render_target->cpu_glyph_tail = &glyph->next;
    return TRUE;
}

static void d2d_wic_render_target_apply_cpu_glyphs(struct d2d_wic_render_target *render_target,
        BYTE *dst, unsigned int dst_pitch)
{
    struct d2d_wic_cpu_glyph *glyph;
    BOOL bgra = render_target->desc.pixelFormat.format == DXGI_FORMAT_B8G8R8A8_UNORM;
    unsigned int x, y;

    for (glyph = render_target->cpu_glyphs; glyph; glyph = glyph->next)
    {
        int left = max(glyph->bounds.left, 0), top = max(glyph->bounds.top, 0);
        int right = min(glyph->bounds.right, (int)render_target->width);
        int bottom = min(glyph->bounds.bottom, (int)render_target->height);

        for (y = top; y < bottom; ++y)
        {
            const BYTE *mask = glyph->values + (y - glyph->bounds.top) * glyph->pitch;
            BYTE *pixel = dst + y * dst_pitch + left * 4;

            for (x = left; x < right; ++x, pixel += 4)
            {
                unsigned int a = mask[x - glyph->bounds.left] * glyph->colour.a + 0.5f;
                unsigned int inv = 255 - a;
                unsigned int r = glyph->colour.r * a + 0.5f;
                unsigned int g = glyph->colour.g * a + 0.5f;
                unsigned int b = glyph->colour.b * a + 0.5f;
                unsigned int ri = bgra ? 2 : 0, bi = bgra ? 0 : 2;

                pixel[ri] = min(255, r + (pixel[ri] * inv + 127) / 255);
                pixel[1] = min(255, g + (pixel[1] * inv + 127) / 255);
                pixel[bi] = min(255, b + (pixel[bi] * inv + 127) / 255);
                if (render_target->desc.pixelFormat.alphaMode == D2D1_ALPHA_MODE_IGNORE)
                    pixel[3] = 255;
                else
                    pixel[3] = min(255, a + (pixel[3] * inv + 127) / 255);
            }
        }
    }
    d2d_wic_render_target_discard_cpu_glyphs(render_target);
}

static HRESULT d2d_wic_render_target_present(IUnknown *outer_unknown)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(outer_unknown);
    D3D10_MAPPED_TEXTURE2D mapped_texture;
    ID3D10Resource *src_resource;
    IWICBitmapLock *bitmap_lock;
    UINT dst_size, dst_pitch;
    ID3D10Device *device;
    WICRect dst_rect;
    BYTE *src, *dst, *dst_base;
    unsigned int i;
    HRESULT hr;

    if (FAILED(hr = IDXGISurface_QueryInterface(render_target->dxgi_surface,
            &IID_ID3D10Resource, (void **)&src_resource)))
    {
        ERR("Failed to get source resource interface, hr %#lx.\n", hr);
        goto end;
    }

    ID3D10Texture2D_GetDevice(render_target->readback_texture, &device);
    ID3D10Device_CopyResource(device, (ID3D10Resource *)render_target->readback_texture, src_resource);
    ID3D10Device_Release(device);
    ID3D10Resource_Release(src_resource);

    dst_rect.X = 0;
    dst_rect.Y = 0;
    dst_rect.Width = render_target->width;
    dst_rect.Height = render_target->height;
    if (FAILED(hr = IWICBitmap_Lock(render_target->bitmap, &dst_rect, WICBitmapLockWrite, &bitmap_lock)))
    {
        ERR("Failed to lock destination bitmap, hr %#lx.\n", hr);
        goto end;
    }

    if (FAILED(hr = IWICBitmapLock_GetDataPointer(bitmap_lock, &dst_size, &dst)))
    {
        ERR("Failed to get data pointer, hr %#lx.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        goto end;
    }

    dst_base = dst;

    if (FAILED(hr = IWICBitmapLock_GetStride(bitmap_lock, &dst_pitch)))
    {
        ERR("Failed to get stride, hr %#lx.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        goto end;
    }

    if (FAILED(hr = ID3D10Texture2D_Map(render_target->readback_texture, 0, D3D10_MAP_READ, 0, &mapped_texture)))
    {
        ERR("Failed to map readback texture, hr %#lx.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        goto end;
    }

    src = mapped_texture.pData;

    for (i = 0; i < render_target->height; ++i)
    {
        memcpy(dst, src, render_target->bpp * render_target->width);
        src += mapped_texture.RowPitch;
        dst += dst_pitch;
    }

    d2d_wic_render_target_apply_cpu_glyphs(render_target, dst_base, dst_pitch);

    ID3D10Texture2D_Unmap(render_target->readback_texture, 0);
    IWICBitmapLock_Release(bitmap_lock);

end:
    d2d_wic_render_target_discard_cpu_glyphs(render_target);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return IUnknown_QueryInterface(render_target->dxgi_inner, iid, out);
}

static ULONG STDMETHODCALLTYPE d2d_wic_render_target_AddRef(IUnknown *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&render_target->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static void d2d_wic_render_target_destroy(struct d2d_wic_render_target *render_target)
{
    d2d_wic_render_target_discard_cpu_glyphs(render_target);
    if (render_target->bitmap)
        IWICBitmap_Release(render_target->bitmap);
    ID3D10Texture2D_Release(render_target->readback_texture);
    IUnknown_Release(render_target->dxgi_inner);
    IDXGISurface_Release(render_target->dxgi_surface);
    free(render_target);
}

static ULONG STDMETHODCALLTYPE d2d_wic_render_target_Release(IUnknown *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&render_target->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        BOOL clean = d2d_device_context_prepare_reuse_target(render_target->dxgi_target);

        if (clean && d2d_wic_pool_enabled()
                && render_target->width <= 32 && render_target->height <= 32)
        {
            struct d2d_wic_render_target *evicted = NULL;

            d2d_wic_render_target_discard_cpu_glyphs(render_target);
            IWICBitmap_Release(render_target->bitmap);
            render_target->bitmap = NULL;
            AcquireSRWLockExclusive(&d2d_wic_pool_lock);
            if (d2d_wic_pool_count == D2D_WIC_POOL_LIMIT)
            {
                evicted = d2d_wic_pool;
                d2d_wic_pool = evicted->pool_next;
                --d2d_wic_pool_count;
            }
            render_target->pool_next = d2d_wic_pool;
            d2d_wic_pool = render_target;
            ++d2d_wic_pool_count;
            ReleaseSRWLockExclusive(&d2d_wic_pool_lock);
            if (evicted)
                d2d_wic_render_target_destroy(evicted);
            return 0;
        }

        d2d_wic_render_target_destroy(render_target);
    }

    return refcount;
}

static const struct IUnknownVtbl d2d_wic_render_target_vtbl =
{
    d2d_wic_render_target_QueryInterface,
    d2d_wic_render_target_AddRef,
    d2d_wic_render_target_Release,
};

static const struct d2d_device_context_ops d2d_wic_render_target_ops =
{
    d2d_wic_render_target_present,
    d2d_wic_render_target_queue_cpu_glyph,
    TRUE,
};

static HRESULT d2d_wic_resolve_pixel_format(D2D1_PIXEL_FORMAT *pixel_format,
        const WICPixelFormatGUID *wic_format)
{
    static const struct
    {
        const WICPixelFormatGUID *wic_format;
        D2D1_PIXEL_FORMAT pixel_format;
    }
    formats[] =
    {
        { &GUID_WICPixelFormat32bppBGR, { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },
        { &GUID_WICPixelFormat32bppRGB, { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },
        { &GUID_WICPixelFormat32bppPBGRA, { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },
        { &GUID_WICPixelFormat32bppPRGBA, { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },
    };

    if (pixel_format->format != DXGI_FORMAT_UNKNOWN && pixel_format->alphaMode != D2D1_ALPHA_MODE_UNKNOWN)
        return S_OK;

    for (int i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        if (IsEqualGUID(formats[i].wic_format, wic_format))
        {
            if (pixel_format->format == DXGI_FORMAT_UNKNOWN)
                pixel_format->format = formats[i].pixel_format.format;
            if (pixel_format->alphaMode == D2D1_ALPHA_MODE_UNKNOWN)
                pixel_format->alphaMode = formats[i].pixel_format.alphaMode;
            return S_OK;
        }
    }

    return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
}

BOOL d2d_wic_render_target_reuse(ID2D1Factory1 *factory, ID3D10Device1 *device,
        IWICBitmap *bitmap, const D2D1_RENDER_TARGET_PROPERTIES *desc, ID2D1RenderTarget **target)
{
    struct d2d_wic_render_target **entry, *render_target;
    D2D1_RENDER_TARGET_PROPERTIES resolved = *desc;
    WICPixelFormatGUID bitmap_format;
    unsigned int width, height;

    if (!d2d_wic_pool_enabled() || FAILED(IWICBitmap_GetSize(bitmap, &width, &height))
            || width > 32 || height > 32
            || FAILED(IWICBitmap_GetPixelFormat(bitmap, &bitmap_format))
            || FAILED(d2d_wic_resolve_pixel_format(&resolved.pixelFormat, &bitmap_format)))
        return FALSE;

    AcquireSRWLockExclusive(&d2d_wic_pool_lock);
    for (entry = &d2d_wic_pool; (render_target = *entry); entry = &render_target->pool_next)
    {
        if (render_target->factory != factory || render_target->device != device
                || render_target->width != width || render_target->height != height
                || render_target->desc.type != resolved.type
                || render_target->desc.pixelFormat.format != resolved.pixelFormat.format
                || render_target->desc.pixelFormat.alphaMode != resolved.pixelFormat.alphaMode
                || render_target->desc.dpiX != resolved.dpiX || render_target->desc.dpiY != resolved.dpiY
                || render_target->desc.usage != resolved.usage || render_target->desc.minLevel != resolved.minLevel)
            continue;

        *entry = render_target->pool_next;
        render_target->pool_next = NULL;
        --d2d_wic_pool_count;
        render_target->bitmap = bitmap;
        IWICBitmap_AddRef(bitmap);
        render_target->refcount = 1;
        *target = render_target->dxgi_target;
        ReleaseSRWLockExclusive(&d2d_wic_pool_lock);

        d2d_device_context_reset_reused_target(*target, resolved.dpiX, resolved.dpiY);
        return TRUE;
    }
    ReleaseSRWLockExclusive(&d2d_wic_pool_lock);
    return FALSE;
}

HRESULT d2d_wic_render_target_init(struct d2d_wic_render_target *render_target, ID2D1Factory1 *factory,
        ID3D10Device1 *d3d_device, IWICBitmap *bitmap, const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    WICPixelFormatGUID bitmap_format;
    ID3D10Texture2D *texture;
    IDXGIDevice *dxgi_device;
    ID2D1Device *device;
    HRESULT hr;

    render_target->IUnknown_iface.lpVtbl = &d2d_wic_render_target_vtbl;

    if (FAILED(hr = IWICBitmap_GetSize(bitmap, &render_target->width, &render_target->height)))
    {
        WARN("Failed to get bitmap dimensions, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IWICBitmap_GetPixelFormat(bitmap, &bitmap_format)))
    {
        WARN("Failed to get bitmap format, hr %#lx.\n", hr);
        return hr;
    }

    rt_desc = *desc;
    if (FAILED(hr = d2d_wic_resolve_pixel_format(&rt_desc.pixelFormat, &bitmap_format)))
    {
        WARN("Unsupported WIC bitmap format %s.\n", debugstr_guid(&bitmap_format));
        return hr;
    }

    texture_desc.Width = render_target->width;
    texture_desc.Height = render_target->height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = rt_desc.pixelFormat.format;

    switch (texture_desc.Format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            render_target->bpp = 4;
            break;

        default:
            FIXME("Unhandled format %#x.\n", texture_desc.Format);
            return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
    }

    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = desc->usage & D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE ?
            D3D10_RESOURCE_MISC_GDI_COMPATIBLE : 0;

    if (FAILED(hr = ID3D10Device1_CreateTexture2D(d3d_device, &texture_desc, NULL, &texture)))
    {
        WARN("Failed to create texture, hr %#lx.\n", hr);
        return hr;
    }

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&render_target->dxgi_surface);
    ID3D10Texture2D_Release(texture);
    if (FAILED(hr))
    {
        WARN("Failed to get DXGI surface interface, hr %#lx.\n", hr);
        return hr;
    }

    texture_desc.Usage = D3D10_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;

    if (FAILED(hr = ID3D10Device1_CreateTexture2D(d3d_device, &texture_desc, NULL, &render_target->readback_texture)))
    {
        WARN("Failed to create readback texture, hr %#lx.\n", hr);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    if (FAILED(hr = ID3D10Device1_QueryInterface(d3d_device, &IID_IDXGIDevice, (void **)&dxgi_device)))
    {
        WARN("Failed to get DXGI device, hr %#lx.\n", hr);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    hr = d2d_factory_create_device(factory, dxgi_device, false, &IID_ID2D1Device, (void **)&device);
    IDXGIDevice_Release(dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to create D2D device, hr %#lx.\n", hr);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    hr = d2d_d3d_create_render_target(unsafe_impl_from_ID2D1Device((ID2D1Device1 *)device),
            render_target->dxgi_surface, &render_target->IUnknown_iface,
            &d2d_wic_render_target_ops, &rt_desc, (void **)&render_target->dxgi_inner);
    ID2D1Device_Release(device);
    if (FAILED(hr))
    {
        WARN("Failed to create DXGI surface render target, hr %#lx.\n", hr);
        ID3D10Texture2D_Release(render_target->readback_texture);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    if (FAILED(hr = IUnknown_QueryInterface(render_target->dxgi_inner,
            &IID_ID2D1RenderTarget, (void **)&render_target->dxgi_target)))
    {
        WARN("Failed to retrieve ID2D1RenderTarget interface, hr %#lx.\n", hr);
        IUnknown_Release(render_target->dxgi_inner);
        ID3D10Texture2D_Release(render_target->readback_texture);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    render_target->factory = factory;
    render_target->device = d3d_device;
    render_target->desc = rt_desc;
    render_target->bitmap = bitmap;
    IWICBitmap_AddRef(bitmap);

    return S_OK;
}
