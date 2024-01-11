/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "win32_common.h"

#include "retroarch.h"

typedef struct ui_window_win32
{
    HWND hwnd;
} ui_window_win32_t;

bool g_win32_inited = false;
unsigned g_win32_resize_width = 0;
unsigned g_win32_resize_height = 0;
bool g_win32_restore_desktop = false;
float g_win32_refresh_rate = 0;
ui_window_win32_t main_window;

static HMONITOR win32_monitor_last;
static HMONITOR win32_monitor_all[MAX_MONITORS];

void win32_set_hwnd(HWND hwnd)
{
	main_window.hwnd = hwnd;
}

typedef struct win32_common_state
{
    int pos_x;
    int pos_y;
    unsigned pos_width;
    unsigned pos_height;
    unsigned taskbar_message;
    unsigned monitor_count;
    bool quit;
    bool resized;
} win32_common_state_t;

static win32_common_state_t win32_st =
{
   CW_USEDEFAULT,       /* pos_x */
   CW_USEDEFAULT,       /* pos_y */
   0,                   /* pos_width */
   0,                   /* pos_height */
   0,                   /* taskbar_message */
   false,               /* quit */
   0,                   /* monitor_count */
   false                /* resized */
};

void win32_check_window(void* data,
    bool* quit, bool* resize,
    unsigned* width, unsigned* height)
{
    win32_common_state_t
        * g_win32 = (win32_common_state_t*)&win32_st;
    *quit = g_win32->quit;

    if (g_win32->resized)
    {
        *resize = true;
        *width = g_win32_resize_width;
        *height = g_win32_resize_height;
        g_win32->resized = false;
    }
}

HWND win32_get_window(void)
{
    return main_window.hwnd;
}

void win32_monitor_info(void* data, void* hm_data, unsigned* mon_id)
{
    unsigned i;
    settings_t* settings = config_get_ptr();
    MONITORINFOEX* mon = (MONITORINFOEX*)data;
    HMONITOR* hm_to_use = (HMONITOR*)hm_data;
    unsigned fs_monitor = 0; // settings->uints.video_monitor_index;
    win32_common_state_t
        * g_win32 = (win32_common_state_t*)&win32_st;

    if (!win32_monitor_last)
        win32_monitor_last = MonitorFromWindow(GetDesktopWindow(),
            MONITOR_DEFAULTTONEAREST);

    *hm_to_use = win32_monitor_last;

    if (fs_monitor && fs_monitor <= g_win32->monitor_count
        && win32_monitor_all[fs_monitor - 1])
    {
        *hm_to_use = win32_monitor_all[fs_monitor - 1];
        *mon_id = fs_monitor - 1;
    }
    else
    {
        for (i = 0; i < g_win32->monitor_count; i++)
        {
            if (win32_monitor_all[i] != *hm_to_use)
                continue;

            *mon_id = i;
            break;
        }
    }

    if (hm_to_use)
    {
        memset(mon, 0, sizeof(*mon));
        mon->cbSize = sizeof(MONITORINFOEX);

        GetMonitorInfo(*hm_to_use, (LPMONITORINFO)mon);
    }
}

void win32_monitor_from_window(void)
{
    win32_monitor_last =
        MonitorFromWindow(main_window.hwnd, MONITOR_DEFAULTTONEAREST);
}

void win32_destroy_window(void)
{
    main_window.hwnd = NULL;
}

int win32_change_display_settings(const char* str, void* devmode_data,
    unsigned flags)
{
#if _WIN32_WINDOWS >= 0x0410 || _WIN32_WINNT >= 0x0410
    /* Windows 98 and later codepath */
    return ChangeDisplaySettingsEx(str, (DEVMODE*)devmode_data,
        NULL, flags, NULL);
#else
    /* Windows 95 / NT codepath */
    return ChangeDisplaySettings((DEVMODE*)devmode_data, flags);
#endif
}

void win32_monitor_get_info(void)
{
    MONITORINFOEX current_mon;

    memset(&current_mon, 0, sizeof(current_mon));
    current_mon.cbSize = sizeof(MONITORINFOEX);

    GetMonitorInfo(win32_monitor_last, (LPMONITORINFO)&current_mon);

    win32_change_display_settings(current_mon.szDevice, NULL, 0);
}

void win32_window_reset(void)
{
    win32_common_state_t
        * g_win32 = (win32_common_state_t*)&win32_st;

    g_win32->quit = false;
    g_win32_restore_desktop = false;
}

static BOOL CALLBACK win32_monitor_enum_proc(HMONITOR hMonitor,
    HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    win32_common_state_t
        * g_win32 = (win32_common_state_t*)&win32_st;

    win32_monitor_all[g_win32->monitor_count++] = hMonitor;
    return TRUE;
}

void win32_monitor_init(void)
{
    win32_common_state_t
        * g_win32 = (win32_common_state_t*)&win32_st;

#if !defined(_XBOX)
    g_win32->monitor_count = 0;
    EnumDisplayMonitors(NULL, NULL,
        win32_monitor_enum_proc, 0);
#endif
    g_win32->quit = false;
}

static bool win32_monitor_set_fullscreen(
    unsigned width, unsigned height,
    unsigned refresh, char* dev_name)
{
#if !defined(_XBOX)
    DEVMODE devmode;

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmPelsWidth = width;
    devmode.dmPelsHeight = height;
    devmode.dmDisplayFrequency = refresh;
    devmode.dmFields = DM_PELSWIDTH
        | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

    RARCH_LOG("[Video]: Setting fullscreen to %ux%u @ %uHz on device %s.\n",
        width, height, refresh, dev_name);

    return win32_change_display_settings(dev_name, &devmode,
        CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
#endif
}

void win32_set_style(MONITORINFOEX* current_mon, HMONITOR* hm_to_use,
    unsigned* width, unsigned* height, bool fullscreen, bool windowed_full,
    RECT* rect, RECT* mon_rect, DWORD* style)
{
#if !defined(_XBOX)
    win32_common_state_t* g_win32 = (win32_common_state_t*)&win32_st;
    bool position_set_from_config = false;
    settings_t* settings = config_get_ptr();
    bool video_window_save_positions = true; //  settings->bools.video_window_save_positions;
    float video_refresh = 30; //  settings->floats.video_refresh_rate;
    unsigned swap_interval = settings->uints.video_swap_interval;
    unsigned window_position_x = settings->uints.window_position_x;
    unsigned window_position_y = settings->uints.window_position_y;
    unsigned window_position_width = settings->uints.window_position_width;
    unsigned window_position_height = settings->uints.window_position_height;

    if (fullscreen)
    {
        /* Windows only reports the refresh rates for modelines as
         * an integer, so video_refresh_rate needs to be rounded. Also, account
         * for black frame insertion using video_refresh_rate set to a portion
         * of the display refresh rate, as well as higher vsync swap intervals. */
        float refresh_mod = 1.0f;
        float refresh_rate = (video_refresh * refresh_mod * swap_interval);

        if (windowed_full)
        {
            *style = WS_EX_TOPMOST | WS_POPUP;
            g_win32_resize_width = *width = mon_rect->right - mon_rect->left;
            g_win32_resize_height = *height = mon_rect->bottom - mon_rect->top;
        }
        else
        {
            *style = WS_POPUP | WS_VISIBLE;

            if (!win32_monitor_set_fullscreen(*width, *height,
                (int)refresh_rate, current_mon->szDevice)) {
            }

            /* Display settings might have changed, get new coordinates. */
            GetMonitorInfo(*hm_to_use, (LPMONITORINFO)current_mon);
            *mon_rect = current_mon->rcMonitor;
        }
    }
    else
    {
        *style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        rect->right = *width;
        rect->bottom = *height;

        AdjustWindowRect(rect, *style, FALSE);

        if (video_window_save_positions)
        {
            /* Set position from config */
            int border_thickness = GetSystemMetrics(SM_CXSIZEFRAME);
            int title_bar_height = GetSystemMetrics(SM_CYCAPTION);

            g_win32->pos_x = window_position_x;
            g_win32->pos_y = window_position_y;
            g_win32->pos_width = window_position_width
                + border_thickness * 2;
            g_win32->pos_height = window_position_height
                + border_thickness * 2 + title_bar_height;

            if (g_win32->pos_width != 0 && g_win32->pos_height != 0)
                position_set_from_config = true;
        }

        if (position_set_from_config)
        {
            g_win32_resize_width = *width = g_win32->pos_width;
            g_win32_resize_height = *height = g_win32->pos_height;
        }
        else
        {
            g_win32_resize_width = *width = rect->right - rect->left;
            g_win32_resize_height = *height = rect->bottom - rect->top;
        }
    }
#endif
}

extern HWND hStatusBar;

void win32_set_window(unsigned* width, unsigned* height,
    bool fullscreen, bool windowed_full, void* rect_data)
{
#if !defined(_XBOX)
    RECT* rect = (RECT*)rect_data;

    if (!fullscreen || windowed_full)
    {
        settings_t* settings = config_get_ptr();
        if (!fullscreen)
        {
            RECT rc_temp;
            rc_temp.left = 0;
            rc_temp.top = 0;
            rc_temp.right = (LONG)*height;
            rc_temp.bottom = 0x7FFF;

            SendMessage(main_window.hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&rc_temp);
            RECT statusRect = { 0 };
            if (hStatusBar)
                GetWindowRect(hStatusBar, &statusRect);

            g_win32_resize_height = *height += rc_temp.top + rect->top;
            int heightOffset = (statusRect.bottom - statusRect.top);
            SetWindowPos(main_window.hwnd, NULL, 0, 0, *width, *height + heightOffset, SWP_NOMOVE);
        }

        ShowWindow(main_window.hwnd, SW_RESTORE);
        UpdateWindow(main_window.hwnd);
        SetForegroundWindow(main_window.hwnd);
    }
#endif
}

bool win32_set_video_mode(void* data,
    unsigned width, unsigned height,
    bool fullscreen)
{
#if !defined(_XBOX)
    DWORD style;
    MSG msg;
    RECT mon_rect;
    RECT rect;
    MONITORINFOEX current_mon;
    int res = 0;
    unsigned mon_id = 0;
    HMONITOR hm_to_use = NULL;
    win32_common_state_t
        * g_win32 = (win32_common_state_t*)&win32_st;
    settings_t* settings = config_get_ptr();
    bool windowed_full = true; // settings->bools.video_windowed_fullscreen;

    rect.left = 0;
    rect.top = 0;
    rect.right = 0;
    rect.bottom = 0;

    win32_monitor_info(&current_mon, &hm_to_use, &mon_id);

    mon_rect = current_mon.rcMonitor;
    g_win32_resize_width = width;
    g_win32_resize_height = height;
    g_win32_refresh_rate = 30; // settings->floats.video_refresh_rate;

    win32_set_style(&current_mon, &hm_to_use, &width, &height,
        fullscreen, windowed_full, &rect, &mon_rect, &style);

#if 0
    if (!win32_window_create(data, style,
        &mon_rect, width, height, fullscreen))
        return false;
#endif

    win32_set_window(&width, &height,
        fullscreen, windowed_full, &rect);

#if 0
    /* Wait until context is created (or failed to do so ...).
     * Please don't remove the (res = ) as GetMessage can return -1. */
    while (!g_win32_inited && !g_win32->quit
        && (res = GetMessage(&msg, main_window.hwnd, 0, 0)) != 0)
    {
        if (res == -1)
        {
            RARCH_ERR("GetMessage error code %d\n", GetLastError());
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_win32->quit)
        return false;
#endif
#endif

    return true;
}

void win32_get_video_output_size(unsigned* width, unsigned* height, char* desc, size_t desc_len)
{
    DebugBreak();
}

float win32_get_refresh_rate(void* data)
{
    return 30.f;
}

bool win32_has_focus(void* data)
{
    if (g_win32_inited)
        if (GetForegroundWindow() == main_window.hwnd)
            return true;

    return false;
}
