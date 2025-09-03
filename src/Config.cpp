#include "stdafx.h"
#include "Config.h"
#include "glN64.h"
#include "Resource.h"
#include "RSP.h"
#include "Textures.h"
#include "OpenGL.h"

HWND hConfigDlg;

#define numWindowedModes 12

struct
{
    WORD width, height;
    char* description;
} windowedModes[numWindowedModes] = {
{320, 240, "320 x 240"},
{400, 300, "400 x 300"},
{480, 360, "480 x 360"},
{640, 480, "640 x 480"},
{800, 600, "800 x 600"},
{960, 720, "960 x 720"},
{1024, 768, "1024 x 768"},
{1152, 864, "1152 x 864"},
{1280, 960, "1280 x 960"},
{1280, 1024, "1280 x 1024"},
{1440, 1080, "1440 x 1080"},
{1600, 1200, "1600 x 1200"}};

void EnableCustom(HWND hWndDlg, BOOL enable)
{
    EnableWindow(GetDlgItem(hWndDlg, IDC_WINDOWED_X), enable);
    EnableWindow(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), enable);
}

void Config_LoadConfig()
{
    DWORD value, size;

    HKEY hKey;

    RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\N64 Emulation\\DLL\\glN64", 0, KEY_READ, &hKey);

    if (hKey)
    {
        RegQueryValueEx(hKey, "Windowed Width", 0, NULL, (BYTE*)&OGL.windowedWidth, &size);
        RegQueryValueEx(hKey, "Windowed Height", 0, NULL, (BYTE*)&OGL.windowedHeight, &size);
        RegQueryValueEx(hKey, "Windowed Width", 0, NULL, (BYTE*)&OGL.windowedWidth, &size);
        RegQueryValueEx(hKey, "Force Bilinear", 0, NULL, (BYTE*)&value, &size);
        OGL.forceBilinear = value ? TRUE : FALSE;

        RegQueryValueEx(hKey, "Texture Filter", 0, NULL, (BYTE*)&value, &size);
        OGL.textureFilter = (TextureFilter)value;

        RegQueryValueEx(hKey, "Filter Scale", 0, NULL, (BYTE*)&value, &size);
        OGL.filterScale = value;

        RegQueryValueEx(hKey, "Enable Fog", 0, NULL, (BYTE*)&value, &size);
        OGL.fog = value ? TRUE : FALSE;

        RegQueryValueEx(hKey, "Texture Cache Size", 0, NULL, (BYTE*)&value, &size);
        cache.maxBytes = value * 1048576;

        RegQueryValueEx(hKey, "Dithered Alpha Testing", 0, NULL, (BYTE*)&value, &size);
        OGL.usePolygonStipple = value ? TRUE : FALSE;

        RegQueryValueEx(hKey, "Ignore Scissor", 0, NULL, (BYTE*)&value, &size);
        OGL.ignoreScissor = value ? TRUE : FALSE;

        RegQueryValueEx(hKey, "Clear Override", 0, NULL, (BYTE*)&value, &size);
        OGL.clear_override = value ? TRUE : FALSE;

        RegQueryValueEx(hKey, "Combiner", 0, NULL, (BYTE*)&value, &size);
        OGL.combiner = value;

        if (OGL.textureFilter == TextureFilter::SaI)
        {
            OGL.filterScale = 2;
        }
        else if (OGL.textureFilter == TextureFilter::Hqx)
        {
            OGL.filterScale = max(2, min(4, OGL.filterScale));
        }
        else if (OGL.textureFilter == TextureFilter::xBRZ)
        {
            OGL.filterScale = max(2, min(6, OGL.filterScale));
        }
        RegCloseKey(hKey);
    }
    else
    {
        OGL.fog = TRUE;
        OGL.windowedWidth = 640;
        OGL.windowedHeight = 480;
        OGL.forceBilinear = FALSE;
        cache.maxBytes = 32 * 1048576;
        OGL.textureFilter = TextureFilter::None;
        OGL.usePolygonStipple = FALSE;
    }
}

void Config_SaveConfig()
{
    DWORD value;
    HKEY hKey;

    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\N64 Emulation\\DLL\\glN64", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);

    RegSetValueEx(hKey, "Windowed Width", 0, REG_DWORD, (BYTE*)&OGL.windowedWidth, 4);
    RegSetValueEx(hKey, "Windowed Height", 0, REG_DWORD, (BYTE*)&OGL.windowedHeight, 4);

    value = OGL.forceBilinear ? 1 : 0;
    RegSetValueEx(hKey, "Force Bilinear", 0, REG_DWORD, (BYTE*)&value, 4);

    value = (DWORD)OGL.textureFilter;
    RegSetValueEx(hKey, "Texture Filter", 0, REG_DWORD, (BYTE*)&value, 4);

    value = OGL.filterScale;
    RegSetValueEx(hKey, "Filter Scale", 0, REG_DWORD, (BYTE*)&value, 4);

    value = OGL.fog ? 1 : 0;
    RegSetValueEx(hKey, "Enable Fog", 0, REG_DWORD, (BYTE*)&value, 4);

    value = cache.maxBytes / 1048576;
    RegSetValueEx(hKey, "Texture Cache Size", 0, REG_DWORD, (BYTE*)&value, 4);

    value = OGL.usePolygonStipple ? 1 : 0;
    RegSetValueEx(hKey, "Dithered Alpha Testing", 0, REG_DWORD, (BYTE*)&value, 4);

    value = OGL.ignoreScissor ? 1 : 0;
    RegSetValueEx(hKey, "Ignore Scissor", 0, REG_DWORD, (BYTE*)&value, 4);

    value = OGL.clear_override ? 1 : 0;
    RegSetValueEx(hKey, "Clear Override", 0, REG_DWORD, (BYTE*)&value, 4);

    value = OGL.combiner;
    RegSetValueEx(hKey, "Combiner", 0, REG_DWORD, (BYTE*)&value, 4);

    RegCloseKey(hKey);
}

void Config_ApplyDlgConfig(HWND hWndDlg)
{
    char text[256];
    int i;

    SendDlgItemMessage(hWndDlg, IDC_CACHEMEGS, WM_GETTEXT, 4, (LPARAM)text);
    cache.maxBytes = atol(text) * 1048576;

    OGL.forceBilinear = (SendDlgItemMessage(hWndDlg, IDC_FORCEBILINEAR, BM_GETCHECK, NULL, NULL) == BST_CHECKED);
    auto filter = OGL.textureFilter;
    OGL.textureFilter = (TextureFilter)SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_GETCURSEL, NULL, NULL);
    if (filter != OGL.textureFilter)
        OGL.filterChanged = TRUE;
    auto filter_scale = OGL.filterScale;
    OGL.filterScale = SendDlgItemMessage(hWndDlg, IDC_FSCALE, TBM_GETPOS, NULL, NULL);
    if (filter_scale != OGL.filterScale)
        OGL.filterChanged = TRUE;

    OGL.fog = (SendDlgItemMessage(hWndDlg, IDC_FOG, BM_GETCHECK, NULL, NULL) == BST_CHECKED);
    OGL.originAdjust = (OGL.textureFilter == TextureFilter::SaI ? 0.25 : 0.50);
    OGL.ignoreScissor = (SendDlgItemMessage(hWndDlg, IDC_SCISSOR, BM_GETCHECK, NULL, NULL) == BST_CHECKED);
    OGL.clear_override = (SendDlgItemMessage(hWndDlg, IDC_CLEAR, BM_GETCHECK, NULL, NULL) == BST_CHECKED);

    OGL.combiner = ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_COMBINER));

    i = SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_GETCURSEL, 0, 0);
    if (i == SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWEDRES), CB_GETCOUNT, 0, 0) - 1)
    {
        char val[32];
        SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWED_X), WM_GETTEXT, 32, (LPARAM)val);
        OGL.windowedWidth = atoi(val);
        SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), WM_GETTEXT, 32, (LPARAM)val);
        OGL.windowedHeight = atoi(val);
    }
    else
    {
        OGL.windowedWidth = windowedModes[i].width;
        OGL.windowedHeight = windowedModes[i].height;
    }

    OGL.usePolygonStipple = (SendDlgItemMessage(hWndDlg, IDC_DITHEREDALPHATEST, BM_GETCHECK, NULL, NULL) == BST_CHECKED);

    OGL_ResizeWindow();
    Config_SaveConfig();
    Config_LoadConfig();
}

BOOL CALLBACK ConfigDlgProc(HWND hWndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char text[256]{};
    bool custom = true;

    switch (message)
    {
    case WM_INITDIALOG:
        hConfigDlg = hWndDlg;


        // Fill windowed mode resolution
        for (int i = 0; i < numWindowedModes; i++)
        {
            SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_ADDSTRING, 0, (LPARAM)windowedModes[i].description);
            if ((OGL.windowedWidth == windowedModes[i].width) &&
                (OGL.windowedHeight == windowedModes[i].height))
            {
                SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_SETCURSEL, i, 0);
                custom = false;
            }
        }
        
        SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_ADDSTRING, 0, (LPARAM) "Custom...");

        char val[32];
        sprintf(val, "%d", OGL.windowedWidth);
        SendDlgItemMessage(hWndDlg, IDC_WINDOWED_X, WM_SETTEXT, 0, (LPARAM)val);
        sprintf(val, "%d", OGL.windowedHeight);
        SendDlgItemMessage(hWndDlg, IDC_WINDOWED_Y, WM_SETTEXT, 0, (LPARAM)val);
        if (custom)
        {
            int num = SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_GETCOUNT, 0, 0) - 1;
            EnableCustom(hWndDlg, TRUE);
            SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_SETCURSEL, num, 0);
        }

        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM) "None");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM) "2xSaI");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM) "xBRZ");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM) "Hqx");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_SETCURSEL, (int)OGL.textureFilter, 0);
        SendMessage(GetDlgItem(hWndDlg, IDC_FSCALE), TBM_SETPOS, TRUE, OGL.filterScale);

        SendDlgItemMessage(hWndDlg, IDC_FORCEBILINEAR, BM_SETCHECK, OGL.forceBilinear ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);
        SendDlgItemMessage(hWndDlg, IDC_SCISSOR, BM_SETCHECK, OGL.ignoreScissor ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);
        SendDlgItemMessage(hWndDlg, IDC_CLEAR, BM_SETCHECK, OGL.clear_override ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);

        ComboBox_AddString(GetDlgItem(hWndDlg, IDC_COMBINER), "Autodetect");
        ComboBox_AddString(GetDlgItem(hWndDlg, IDC_COMBINER), "TEXTURE_ENV");
        ComboBox_AddString(GetDlgItem(hWndDlg, IDC_COMBINER), "TEXTURE_ENV_COMBINE");
        ComboBox_AddString(GetDlgItem(hWndDlg, IDC_COMBINER), "NV_REGISTER_COMBINERS");
        ComboBox_SetCurSel(GetDlgItem(hWndDlg, IDC_COMBINER), OGL.combiner);

        // Enable/disable fog
        SendDlgItemMessage(hWndDlg, IDC_FOG, BM_SETCHECK, OGL.fog ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);

        SendDlgItemMessage(hWndDlg, IDC_DITHEREDALPHATEST, BM_SETCHECK, OGL.usePolygonStipple ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);

        _ltoa(cache.maxBytes / 1048576, text, 10);
        SendDlgItemMessage(hWndDlg, IDC_CACHEMEGS, WM_SETTEXT, NULL, (LPARAM)text);

        SendMessage(hWndDlg, WM_COMMAND,
                    MAKEWPARAM(IDC_TEXTUREFILTER, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hWndDlg, IDC_TEXTUREFILTER));

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            Config_ApplyDlgConfig(hWndDlg);
            EndDialog(hWndDlg, wParam);
            hConfigDlg = NULL;
            return TRUE;
        case IDCANCEL:
            EndDialog(hWndDlg, wParam);
            hConfigDlg = NULL;
            return TRUE;
        case IDC_WINDOWEDRES:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                if (SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_GETCURSEL, 0, 0) == SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_GETCOUNT, 0, 0) - 1)
                {
                    EnableCustom(hWndDlg, TRUE);
                }
                else
                {
                    EnableCustom(hWndDlg, FALSE);
                }
            }
        case IDC_TEXTUREFILTER:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                const auto filter = (TextureFilter)SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_GETCURSEL, 0, 0);

                int min_scale = 0;
                int max_scale = 0;
                switch (filter)
                {
                case TextureFilter::SaI:
                    min_scale = max_scale = 2;
                    break;
                case TextureFilter::xBRZ:
                    min_scale = 2;
                    max_scale = 6;
                    break;
                case TextureFilter::Hqx:
                    min_scale = 2;
                    max_scale = 4;
                    break;
                default:
                    break;
                }

                SendMessage(GetDlgItem(hWndDlg, IDC_FSCALE), TBM_SETRANGE, TRUE, MAKELONG(min_scale, max_scale));
                EnableWindow(GetDlgItem(hWndDlg, IDC_FSCALE), min_scale != max_scale);
            }
        }
    }
    return FALSE;
}

void Config_Show(HWND parent)
{
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIGDLG), parent, (DLGPROC)ConfigDlgProc);
}
