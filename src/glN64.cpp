#include "stdafx.h"
#include "glN64.h"
#include "OpenGL.h"
#include "N64.h"
#include "RSP.h"
#include "Config.h"

HWND hWnd;
HWND hStatusBar;
HWND hToolBar;
HINSTANCE hInstance;

char* screenDirectory;

void (*CheckInterrupts)(void);

LONG windowedStyle;
LONG windowedExStyle;
RECT windowedRect;
HMENU windowedMenu;

bool init_rsp_thread()
{
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_START]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
        return true;
    }

    for (auto& i : RSP.threadMsg)
    {
        i = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (i == nullptr)
        {
            MessageBox(hWnd, "Error creating video thread message events.", PLUGIN_NAME, MB_OK | MB_ICONERROR);
            return false;
        }
    }

    RSP.threadFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (RSP.threadFinished == NULL)
    {
        MessageBox(hWnd, "Error creating video thread finished event.", PLUGIN_NAME, MB_OK | MB_ICONERROR);
        return false;
    }

    RSP.halt = FALSE;

    DWORD thread_id;
    RSP.thread = CreateThread(NULL, 4096, RSP_ThreadProc, NULL, NULL, &thread_id);
    WaitForSingleObject(RSP.threadFinished, INFINITE);

    SetEvent(RSP.threadMsg[RSPMSG_START]);
    WaitForSingleObject(RSP.threadFinished, INFINITE);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    hInstance = hinstDLL;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        Config_LoadConfig();
        RSP.thread = NULL;
        OGL.hRC = NULL;
        OGL.hDC = NULL;
    }
    return TRUE;
}

EXPORT void CALL CaptureScreen(char* Directory)
{
    screenDirectory = Directory;
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_CAPTURESCREEN]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}

EXPORT void CALL ChangeWindow(void)
{
    // Textures seem to get corrupted when changing video modes (at least on my Radeon), so destroy them
    SetEvent(RSP.threadMsg[RSPMSG_DESTROYTEXTURES]);
    WaitForSingleObject(RSP.threadFinished, INFINITE);

    if (!OGL.fullscreen)
    {
        DEVMODE fullscreenMode;
        memset(&fullscreenMode, 0, sizeof(DEVMODE));
        fullscreenMode.dmSize = sizeof(DEVMODE);
        fullscreenMode.dmPelsWidth = OGL.fullscreenWidth;
        fullscreenMode.dmPelsHeight = OGL.fullscreenHeight;
        fullscreenMode.dmBitsPerPel = OGL.fullscreenBits;
        fullscreenMode.dmDisplayFrequency = OGL.fullscreenRefresh;
        fullscreenMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

        if (ChangeDisplaySettings(&fullscreenMode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            MessageBox(NULL, "Failed to change display mode", PLUGIN_NAME, MB_ICONERROR | MB_OK);
            return;
        }

        ShowCursor(FALSE);

        windowedMenu = GetMenu(hWnd);

        if (windowedMenu)
            SetMenu(hWnd, NULL);

        if (hStatusBar)
            ShowWindow(hStatusBar, SW_HIDE);

        windowedExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        windowedStyle = GetWindowLong(hWnd, GWL_STYLE);

        SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
        SetWindowLong(hWnd, GWL_STYLE, WS_POPUP);

        GetWindowRect(hWnd, &windowedRect);

        OGL.fullscreen = TRUE;
        OGL_ResizeWindow();
    }
    else
    {
        ChangeDisplaySettings(NULL, 0);

        ShowCursor(TRUE);

        if (windowedMenu)
            SetMenu(hWnd, windowedMenu);

        if (hStatusBar)
            ShowWindow(hStatusBar, SW_SHOW);

        SetWindowLong(hWnd, GWL_STYLE, windowedStyle);
        SetWindowLong(hWnd, GWL_EXSTYLE, windowedExStyle);
        SetWindowPos(hWnd, NULL, windowedRect.left, windowedRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        OGL.fullscreen = FALSE;
        OGL_ResizeWindow();
    }

    SetEvent(RSP.threadMsg[RSPMSG_INITTEXTURES]);
    WaitForSingleObject(RSP.threadFinished, INFINITE);
}

EXPORT void CALL DllAbout(void* hParent)
{
    const auto msg = PLUGIN_NAME "\nPart of the Mupen64 project family.\n\nhttps://github.com/mupen64/bettergln";
    MessageBox((HWND)hParent, msg, PLUGIN_NAME, MB_OK | MB_ICONINFORMATION);
}

EXPORT void CALL DllConfig(void* hParent)
{
    Config_Show((HWND)hParent);
}

EXPORT void CALL GetDllInfo(core_plugin_info* PluginInfo)
{
    PluginInfo->ver = 0x100;
    PluginInfo->type = plugin_video;
    strcpy(PluginInfo->name, PLUGIN_NAME);
    PluginInfo->unused_normal_memory = FALSE;
    PluginInfo->unused_byteswapped = TRUE;
}

BOOL CALLBACK FindToolBarProc(HWND hWnd, LPARAM lParam)
{
    if (GetWindowLong(hWnd, GWL_STYLE) & RBS_VARHEIGHT)
    {
        hToolBar = hWnd;
        return FALSE;
    }
    return TRUE;
}

EXPORT BOOL CALL InitiateGFX(core_gfx_info Gfx_Info)
{
    // HACK: Detect when we're being called to prepare for dll config routine
    if (Gfx_Info.main_hwnd == Gfx_Info.statusbar_hwnd)
    {
        return TRUE;
    }

    hWnd = (HWND)Gfx_Info.main_hwnd;
    hStatusBar = (HWND)Gfx_Info.statusbar_hwnd;
    hToolBar = NULL;

    // If the mupen window has CS_OWNDC, we can recycle one DC for wgl and avoid recreating the context when resetting
    const ULONG_PTR class_style = GetClassLongPtr(hWnd, GCL_STYLE);
    OGL.recycle_context = (class_style & CS_OWNDC) != 0;

    EnumChildWindows(hWnd, FindToolBarProc, 0);

    DMEM = Gfx_Info.dmem;
    IMEM = Gfx_Info.imem;
    RDRAM = Gfx_Info.rdram;

    REG.MI_INTR = Gfx_Info.mi_intr_reg;
    REG.DPC_START = Gfx_Info.dpc_start_reg;
    REG.DPC_END = Gfx_Info.dpc_end_reg;
    REG.DPC_CURRENT = Gfx_Info.dpc_current_reg;
    REG.DPC_STATUS = Gfx_Info.dpc_status_reg;
    REG.DPC_CLOCK = Gfx_Info.dpc_clock_reg;
    REG.DPC_BUFBUSY = Gfx_Info.dpc_bufbusy_reg;
    REG.DPC_PIPEBUSY = Gfx_Info.dpc_pipebusy_reg;
    REG.DPC_TMEM = Gfx_Info.dpc_tmem_reg;

    REG.VI_STATUS = Gfx_Info.vi_status_reg;
    REG.VI_ORIGIN = Gfx_Info.vi_origin_reg;
    REG.VI_WIDTH = Gfx_Info.vi_width_reg;
    REG.VI_INTR = Gfx_Info.vi_intr_reg;
    REG.VI_V_CURRENT_LINE = Gfx_Info.vi_v_current_line_reg;
    REG.VI_TIMING = Gfx_Info.vi_timing_reg;
    REG.VI_V_SYNC = Gfx_Info.vi_v_sync_reg;
    REG.VI_H_SYNC = Gfx_Info.vi_h_sync_reg;
    REG.VI_LEAP = Gfx_Info.vi_leap_reg;
    REG.VI_H_START = Gfx_Info.vi_h_start_reg;
    REG.VI_V_START = Gfx_Info.vi_v_start_reg;
    REG.VI_V_BURST = Gfx_Info.vi_v_burst_reg;
    REG.VI_X_SCALE = Gfx_Info.vi_x_scale_reg;
    REG.VI_Y_SCALE = Gfx_Info.vi_y_scale_reg;

    CheckInterrupts = Gfx_Info.check_interrupts;

    if (!init_rsp_thread())
    {
        return FALSE;
    }

    return TRUE;
}

EXPORT void CALL MoveScreen(int xpos, int ypos)
{
}

EXPORT void CALL ProcessDList(void)
{
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_PROCESSDLIST]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}

EXPORT void CALL ProcessRDPList(void)
{
}

EXPORT void CALL RomClosed(void)
{
    if (RSP.thread)
    {
        if (RSP.busy)
        {
            RSP.halt = TRUE;
            WaitForSingleObject(RSP.threadFinished, INFINITE);
        }

        SetEvent(RSP.threadMsg[RSPMSG_CLOSE]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}

EXPORT void CALL RomOpen(void)
{
    OGL_ResizeWindow();
}

EXPORT void CALL ShowCFB(void)
{
}

EXPORT void CALL UpdateScreen(void)
{
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_UPDATESCREEN]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}

EXPORT void CALL ViStatusChanged(void)
{
}

EXPORT void CALL ViWidthChanged(void)
{
}

// not to confuse with readscreen2 from mupen64plus specs (I think)
EXPORT void CALL ReadScreen2(void** dest, long* width, long* height)
{
    extern void* gCapturedPixels;
    *width = OGL.width;
    *height = OGL.height;

    *dest = malloc(OGL.height * OGL.width * 3);
    if (*dest == 0)
        return;
    gCapturedPixels = *dest;
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_READPIXELS]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}

void CALL mge_get_video_size(long* width, long* height)
{
    *width = OGL.width;
    *height = OGL.height;
}

void CALL mge_read_video(void** buffer)
{
    extern void* gCapturedPixels;
    gCapturedPixels = *buffer;
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_READPIXELS]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}
