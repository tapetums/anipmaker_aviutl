//---------------------------------------------------------------------------//
//
// anipmaker.c
//  Copyright (C) 2018 tapetums
//
//---------------------------------------------------------------------------//

#include <windows.h>

#include <output.h>

//---------------------------------------------------------------------------//
// Plugin Information
//---------------------------------------------------------------------------//

OUTPUT_PLUGIN_TABLE output_plugin_table =
{
    0,
    "アニメーションWebP出力",
    "WebP file (*.webp)\0*.webp\0All files (*.*)\0*.*\0",
    "アニメーションWebP出力 version 0.1.0.0 by tapetums",
    NULL,
    NULL,
    func_output,
    func_config,
    func_config_get,
    func_config_set,
};

//---------------------------------------------------------------------------//
// Output Plugin Functions
//---------------------------------------------------------------------------//

EXTERN_C OUTPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetOutputPluginTable(void)
{
    return &output_plugin_table;
}

//---------------------------------------------------------------------------//
// Output Plugin Functions
//---------------------------------------------------------------------------//

BOOL func_output(OUTPUT_INFO* oip)
{
    UNREFERENCED_PARAMETER(oip);

    return TRUE;
}

//---------------------------------------------------------------------------//

BOOL func_config(HWND hwnd, HINSTANCE hinst)
{
    UNREFERENCED_PARAMETER(hinst);

    MessageBoxA(hwnd, output_plugin_table.information, output_plugin_table.name, MB_OK);

    return TRUE;
}

//---------------------------------------------------------------------------//

int func_config_get(void* data, int size)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(size);

    return 0;
}

//---------------------------------------------------------------------------//

int func_config_set(void* data, int size)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(size);

    return 0;
}

//---------------------------------------------------------------------------//

// anipmaker.c