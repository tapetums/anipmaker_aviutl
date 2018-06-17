//---------------------------------------------------------------------------//
//
// anipmaker.cpp
//  Copyright (C) 2018 tapetums
//
//---------------------------------------------------------------------------//

#include <vector>

#include <windows.h>
#include <strsafe.h>

#include <output.h>

#include "config.hpp"
#include "settings.hpp"

#if INTPTR_MAX == INT32_MAX
  #if defined(_DEBUG) || defined(DEBUG)
    #pragma comment(lib, "lib/x86/libwebp_debug.lib")
    #pragma comment(lib, "lib/x86/libwebpmux_debug.lib")
  #else
    #pragma comment(lib, "lib/x86/libwebp.lib")
    #pragma comment(lib, "lib/x86/libwebpmux.lib")
  #endif
#elif INTPTR_MAX == INT64_MAX
  #if defined(_DEBUG) || defined(DEBUG)
    #pragma comment(lib, "lib/x64/libwebp_debug.lib")
    #pragma comment(lib, "lib/x64/libwebpmux_debug.lib")
  #else
    #pragma comment(lib, "lib/x64/libwebp.lib")
    #pragma comment(lib, "lib/x64/libwebpmux.lib")
  #endif
#else
  #error Unknown architecture
#endif

//---------------------------------------------------------------------------//
// Global Variables
//---------------------------------------------------------------------------//

LPCSTR    g_appname { "Animated WebP Export" };
LPCSTR    g_version { "0.1.0.0" };
HINSTANCE g_hInst;

//---------------------------------------------------------------------------//
// Plugin Information
//---------------------------------------------------------------------------//

char name        [256];
char filefilter  [256];
char information [256];

OUTPUT_PLUGIN_TABLE output_plugin_table =
{
    0,
    LPSTR("Animated WebP Export"),
    LPSTR("WebP file (*.webp)\0*.webp\0All files (*.*)\0*.*\0"),
    LPSTR("Animated WebP Export version 0.1.0.0 by tapetums"),
    nullptr,
    nullptr,
    func_output,
    func_config,
    func_config_get,
    func_config_set,
};

EXTERN_C OUTPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetOutputPluginTable()
{
    return &output_plugin_table;
}

//---------------------------------------------------------------------------//
// RAII Classes
//---------------------------------------------------------------------------//

struct WebPPictureGuard
{
    WebPPicture& picture;

    WebPPictureGuard
    (
        WebPPicture& picture, int32_t width, int32_t height
    )
    : picture(picture)
    {
        const auto init = WebPPictureInit(&picture);
        if ( ! init ) { throw; }

        picture.width    = width;
        picture.height   = height;
        picture.use_argb = 1;
    }

    ~WebPPictureGuard()
    {
        WebPPictureFree(&picture);
    }
};

//---------------------------------------------------------------------------//
// Utility Functions
//---------------------------------------------------------------------------//

void flip_image
(
    const void* src, int stride, int height, std::vector<uint8_t>* img
)
{
    auto i = 0;
    auto p = (const uint8_t*)src + stride * (height - 1);

    while ( p >= src )
    {
        ::memcpy(img->data() + i, p, stride);

        i += stride;
        p -= stride;
    }
}

//---------------------------------------------------------------------------//
// Output Plugin Functions
//---------------------------------------------------------------------------//

BOOL func_output(OUTPUT_INFO* oip)
{
    WebPAnimEncoderOptions options;
    {
        const auto init = WebPAnimEncoderOptionsInit(&options);
        if ( ! init ) { return FALSE; }

        options = g_config.options;
    }

    const auto encoder = WebPAnimEncoderNew(oip->w, oip->h, &options);
    if ( encoder == nullptr ) { return FALSE; }

    WebPConfig config;
    {
        const auto init = WebPConfigInit(&config);
        if ( ! init ) { return FALSE; }

        config = g_config.config;

        const auto valid = WebPValidateConfig(&config);
        if ( ! valid ) { return FALSE; }
    }

    const auto stride     = (((oip->w * 24) + 31) & ~31) / 8;
    const auto duration   = int32_t(1000.0 * oip->scale / oip->rate);
    const auto frame_skip = g_config.frame_skip;

    int32_t skip { 0 };
    int32_t timestamp_ms { 0 };

    std::vector<uint8_t> img;
    img.reserve(stride * oip->h);

    for ( auto i = 0; i < oip->n; ++i )
    {
        if ( oip->func_is_abort() ) { break; }

        oip->func_rest_time_disp(i, oip->n);

        flip_image(oip->func_get_video_ex(i, 0), stride, oip->h, &img);

        if ( skip > 0 )
        {
            timestamp_ms += duration;

            --skip;

            continue;
        }

        WebPPicture picture;
        WebPPictureGuard guard(picture, oip->w, oip->h);

        const auto imported = WebPPictureImportBGR
        (
            &picture, img.data(), stride
        );
        if ( ! imported ) { continue; }

        const auto added = WebPAnimEncoderAdd
        (
            encoder, &picture, timestamp_ms, &config
        );
        if ( ! added ) { continue; }

        timestamp_ms += duration;

        if ( skip <= 0 )
        {
            skip = frame_skip;
        }

        oip->func_update_preview();
    }

    WebPAnimEncoderAdd(encoder, nullptr, timestamp_ms, nullptr);

    const auto fp = ::CreateFileA
    (
        oip->savefile, GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
    );
    if ( fp == INVALID_HANDLE_VALUE ) { return FALSE; }

    WebPData webp_data;
    {
        const auto assembled = WebPAnimEncoderAssemble(encoder, &webp_data);
        WebPAnimEncoderDelete(encoder);
        if ( ! assembled ) { ::CloseHandle(fp); return FALSE; }

        DWORD dw;
        WriteFile(fp, webp_data.bytes, DWORD(webp_data.size), &dw, nullptr);

        WebPDataClear(&webp_data);
    }

    ::CloseHandle(fp);
    
    return TRUE;
}

//---------------------------------------------------------------------------//

BOOL func_config(HWND hwnd, HINSTANCE hinst)
{
    UNREFERENCED_PARAMETER(hinst);

    SettingWnd setting_wnd(hwnd);
    setting_wnd.is_alive = true;

    MSG msg { };
    while ( setting_wnd.is_alive )
    {
        ::GetMessage(&msg, nullptr, 0, 0);
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return TRUE;
}

//---------------------------------------------------------------------------//

int func_config_get(void* data, int size)
{
    UNREFERENCED_PARAMETER(size);

    if ( data )
    {
        ::memcpy(data, &g_config, sizeof(g_config));
    }

    return sizeof(g_config);
}

//---------------------------------------------------------------------------//

int func_config_set(void* data, int size)
{
    if ( size != sizeof(g_config) )
    {
        return 0;
    }

    if ( data )
    {
        ::memcpy(&g_config, data, size);
    }

    return 0;
}

//---------------------------------------------------------------------------//

// anipmaker.cpp