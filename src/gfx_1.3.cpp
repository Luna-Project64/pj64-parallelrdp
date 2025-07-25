/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-video-angrylionplus - plugin.c                            *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define M64P_PLUGIN_PROTOTYPES 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "gfx_1.3.h"
#include "parallel_imp.h"
#include "ini.h"
#include "config_gui.h"
#include "config.h"
#include "queue_executor.h"

#include "git.h"

static bool warn_hle = false;
GFX_INFO gfx;
uint32_t rdram_size;
static QueueExecutor sExecutor;
extern "C"
{
    HWND hStatusBar;
}

#define MSG_BUFFER_LEN 256

static void msg_error(const char* err, ...)
{
    va_list arg;
    va_start(arg, err);
    char buf[MSG_BUFFER_LEN];
    vsprintf_s(buf, sizeof(buf), err, arg);
    MessageBoxA(0, buf, "paraLLEl: fatal error", MB_OK);
    va_end(arg);
    exit(0);
}

static void msg_warning(const char* err, ...)
{
    va_list arg;
    va_start(arg, err);
    char buf[MSG_BUFFER_LEN];
    vsprintf_s(buf, sizeof(buf), err, arg);
    MessageBox(0, buf, "paraLLEl: warning", MB_OK);
    va_end(arg);
}

static void msg_debug(const char* err, ...)
{
    va_list arg;
    va_start(arg, err);
    char buf[MSG_BUFFER_LEN];
    vsprintf_s(buf, sizeof(buf), err, arg);
    strcat_s(buf, sizeof(buf), "\n");
    OutputDebugStringA(buf);
    va_end(arg);
}

static bool is_valid_ptr(void *ptr, uint32_t bytes)
{
    SIZE_T dwSize;
    MEMORY_BASIC_INFORMATION meminfo;
    if (!ptr) {
        return false;
    }
    memset(&meminfo, 0x00, sizeof(meminfo));
    dwSize = VirtualQuery(ptr, &meminfo, sizeof(meminfo));
    if (!dwSize) {
        return false;
    }
    if (MEM_COMMIT != meminfo.State) {
        return false;
    }
    if (!(meminfo.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
        return false;
    }
    if (bytes > meminfo.RegionSize) {
        return false;
    }
    if ((uint64_t)((char*)ptr - (char*)meminfo.BaseAddress) > (uint64_t)(meminfo.RegionSize - bytes)) {
        return false;
    }
    return true;
}

void plugin_init(void)
{
    rdram_size = 0x800000;
    // Zilmar's API doesn't provide a way to check the amount of RDRAM available.
    // It can only be 4 MiB or 8 MiB, so check if the last 16 bytes of the provided
    // buffer in the 8 MiB range are valid. If not, it must be 4 MiB.
    if (!is_valid_ptr(&gfx.RDRAM[0x7f0000], 16)) {
        rdram_size /= 2;
    }
}

void plugin_close(void)
{
}

EXPORT void CALL CaptureScreen(char* directory)
{

}

EXPORT void CALL GetDllInfo(PLUGIN_INFO* PluginInfo)
{
    // Copy substring of git hash into buffer.
    char hash[8] = {0};
    strncpy(hash, GIT_HEAD_SHA1, 7);

    PluginInfo->Version = 0x0103;
    PluginInfo->Type  = PLUGIN_TYPE_GFX;
    snprintf(PluginInfo->Name, sizeof(PluginInfo->Name), "LINK's ParaLLEl-RDP rev.%s", hash);

    PluginInfo->NormalMemory = TRUE;
    PluginInfo->MemoryBswaped = TRUE;
}

EXPORT BOOL CALL InitiateGFX(GFX_INFO Gfx_Info)
{
    // initialize config
    config_init();

    gfx = Gfx_Info;
    hStatusBar = gfx.hStatusBar;
    plugin_init();
    return TRUE;
}

EXPORT void CALL CloseDLL(void)
{
}

EXPORT void CALL MoveScreen(int xpos, int ypos)
{
}

EXPORT void CALL ProcessDList(void)
{
    if (!warn_hle) {
        msg_warning("Please disable 'Graphic HLE' in the plugin settings.");
        warn_hle = true;
    }
}

EXPORT void CALL ProcessRDPList(void)
{
    sExecutor.sync([]()
	{
        RDP::begin_frame();
        RDP::process_commands();
    });
}

extern "C" void win32_set_hwnd(HWND hwnd);

static bool m_fullscreen = false;
static int m_width, m_height;
static void init()
{
    RDP::upscaling = settings[KEY_UPSCALING].val;
    RDP::super_sampled_read_back = settings[KEY_SSREADBACKS].val;
    RDP::super_sampled_dither = settings[KEY_SSDITHER].val;

    RDP::interlacing = settings[KEY_DEINTERLACE].val;
    RDP::overscan = settings[KEY_OVERSCANCROP].val;
    RDP::native_texture_lod = settings[KEY_NATIVETEXTLOD].val;
    RDP::native_tex_rect = settings[KEY_NATIVETEXTRECT].val;
    RDP::divot_filter = settings[KEY_DIVOT].val;
    RDP::gamma_dither = settings[KEY_GAMMADITHER].val;
    RDP::dither_filter = settings[KEY_VIDITHER].val;
    RDP::interlacing = settings[KEY_DEINTERLACE].val;
    RDP::vi_aa = settings[KEY_AA].val;
    RDP::vi_scale = settings[KEY_VIBILERP].val;
    RDP::downscaling_steps = settings[KEY_DOWNSCALING].val;
    RDP::synchronous = settings[KEY_SYNCHRONOUS].val;

    if (!m_fullscreen)
    {
        m_width = settings[KEY_SCREEN_WIDTH].val;
		m_height = settings[KEY_SCREEN_HEIGHT].val;
	}

    win32_set_hwnd(gfx.hWnd);
    retro_init(m_fullscreen, m_width, m_height);
}

EXPORT void CALL DllConfig(HWND hParent)
{
    config_gui_open(hParent);
    sExecutor.async([]()
        {
            // reload settings
            config_load();
            retro_deinit();
            init();
        });
}

EXPORT void CALL RomOpen(void)
{
    // Vulkan does not seem to be particularly happy about multithreading either although it might work
    sExecutor.start(true /*same thread exec*/);
    sExecutor.sync(init);
}

EXPORT void CALL DrawScreen(void)
{
}

EXPORT void CALL ReadScreen(void **dest, long *width, long *height)
{
}

EXPORT void CALL RomClosed(void)
{
    sExecutor.async(retro_deinit);
    sExecutor.stop();
}

EXPORT void CALL ShowCFB(void)
{
    sExecutor.async([]() {
        RDP::complete_frame();
        RDP::profile_refresh_begin();
        retro_video_refresh(RETRO_HW_FRAME_BUFFER_VALID, RDP::width, RDP::height, 0);
        RDP::profile_refresh_end();
    });
}

EXPORT void CALL UpdateScreen(void)
{
    ShowCFB();
}

EXPORT void CALL ViStatusChanged(void)
{
}

EXPORT void CALL ViWidthChanged(void)
{
}

static void screen_toggle_fullscreen()
{
    static HMENU old_menu;
    static LONG old_style;
    static WINDOWPLACEMENT old_pos;

    m_fullscreen = !m_fullscreen;

    if (m_fullscreen) {
        // hide curser
        ShowCursor(FALSE);

        // hide status bar
        if (hStatusBar) {
            ShowWindow(hStatusBar, SW_HIDE);
        }

        // disable menu and save it to restore it later
        old_menu = GetMenu(gfx.hWnd);
        if (old_menu) {
            SetMenu(gfx.hWnd, NULL);
        }

        // save old window position and size
        GetWindowPlacement(gfx.hWnd, &old_pos);

        // use virtual screen dimensions for fullscreen mode
        m_width = GetSystemMetrics(SM_CXSCREEN);
        m_height = GetSystemMetrics(SM_CYSCREEN);

        // disable all styles to get a borderless window and save it to restore
        // it later
        old_style = GetWindowLong(gfx.hWnd, GWL_STYLE);
        LONG style = WS_VISIBLE;
        SetWindowLong(gfx.hWnd, GWL_STYLE, style);

        // resize window so it covers the entire virtual screen
        SetWindowPos(gfx.hWnd, HWND_TOP, 0, 0, m_width, m_height, SWP_SHOWWINDOW);
    }
    else {
        // restore cursor
        ShowCursor(TRUE);

        // restore status bar
        if (hStatusBar) {
            ShowWindow(hStatusBar, SW_SHOW);
        }

        // restore menu
        if (old_menu) {
            SetMenu(gfx.hWnd, old_menu);
            old_menu = NULL;
        }

        // restore style
        SetWindowLong(gfx.hWnd, GWL_STYLE, old_style);

        // restore window size and position
        SetWindowPlacement(gfx.hWnd, &old_pos);
    }
    retro_deinit();
    init();
}

EXPORT void CALL ChangeWindow(void)
{
    sExecutor.async(screen_toggle_fullscreen);
}

EXPORT void CALL FBWrite(DWORD addr, DWORD size)
{
}

EXPORT void CALL FBRead(DWORD addr)
{
}

EXPORT void CALL FBGetFrameBufferInfo(void *pinfo)
{
}

EXPORT BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    // set hInstance for the config GUI
    config_gui_hInstance = hModule;
    return TRUE;
}
