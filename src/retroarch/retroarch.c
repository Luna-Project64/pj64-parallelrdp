/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2021 - Daniel De Matteis
 *  Copyright (C) 2012-2015 - Michael Lelli
 *  Copyright (C) 2014-2017 - Jean-Andr� Santoni
 *  Copyright (C) 2016-2019 - Brad Parker
 *  Copyright (C) 2016-2019 - Andr�s Su�rez (input mapper code)
 *  Copyright (C) 2016-2017 - Gregor Richards (network code)
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

#include "retroarch.h"

#include "../config.h"
#include "driver.h"
#include "video_driver.h"

#include <stdio.h>

#include <Windows.h>

extern bool parallel_retro_init_vulkan(void);

retro_log_printf_t log_cb;

void retro_video_refresh(const void* data, unsigned width, unsigned height, size_t pitch)
{
    video_driver_frame(data, width, height, pitch);
}

void retro_log(enum retro_log_level level, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
    char buf[512];
    sprintf(buf, fmt, va);
    OutputDebugString(buf);
	va_end(va);
}

#define log retro_log

void retroarch_fail(int num, const char* err, ...)
{
    va_list arg;
    va_start(arg, err);
    char buf[256];
    vsprintf_s(buf, sizeof(buf), err, arg);
    MessageBox(0, buf, "paraLLEl: warning", MB_OK);
    va_end(arg);
}

static bool core_init() 
{
    video_driver_set_cached_frame_ptr(NULL);
    return !parallel_retro_init_vulkan();
}

static bool core_unload_game(void)
{
    video_driver_free_hw_context();
    video_driver_set_cached_frame_ptr(NULL);
    return true;
}

static bool core_deinit()
{
    video_driver_state_t
        * video_st = video_state_get_ptr();

    core_unload_game();

    video_driver_set_cached_frame_ptr(NULL);

    driver_uninit(DRIVERS_CMD_ALL);
}

bool retro_init()
{
	log_cb = log;
    settings_t* rsettings = config_get_ptr();

    rsettings->uints.window_position_height = settings[KEY_SCREEN_HEIGHT].val;
    rsettings->uints.window_position_width = settings[KEY_SCREEN_WIDTH].val;

#if defined(DEBUG) && defined(HAVE_DRMINGW)
    char log_file_name[128];
#endif
    bool verbosity_enabled = false;
    bool           init_failed = false;
    video_driver_state_t* video_st = video_state_get_ptr();
    video_st->active = true;

    if (!video_driver_find_driver(rsettings,
        "video driver", verbosity_enabled))
        retroarch_fail(1, "video_driver_find_driver()");

    init_failed = core_init();

    /* Handle core initialization failure */
    if (init_failed)
    {
        retroarch_fail(1, "core_init()");
        goto error;
    }

    drivers_init(rsettings, DRIVERS_CMD_ALL, verbosity_enabled);
    return true;

error:
    core_deinit();
    return false;
}

void retro_deinit(void)
{
    core_deinit();
}

void retro_reinit()
{
    video_driver_reinit(0);
}

static settings_t config_st = { 0 };

settings_t* config_get_ptr(void)
{
    return &config_st;
}

void retro_sleep(int amt)
{
    Sleep(amt);
}

void* retro_get_hw_render_interface()
{
    video_driver_state_t* video_st = video_state_get_ptr();
    const struct retro_hw_render_interface* iface = NULL;
    if (
        video_st->poke
        && video_st->poke->get_hw_render_interface
        && video_st->poke->get_hw_render_interface(
            video_st->data, &iface))
        return iface;

    return NULL;
}

bool retro_set_hw_render(void* data) 
{
    struct retro_hw_render_callback* cb =
        (struct retro_hw_render_callback*)data;
    video_driver_state_t* video_st =
        video_state_get_ptr();
    struct retro_hw_render_callback* hwr =
        VIDEO_DRIVER_GET_HW_CONTEXT_INTERNAL(video_st);

    if (!cb)
    {
        RARCH_ERR("[Environ]: SET_HW_RENDER - No valid callback passed, returning...\n");
        return false;
    }

    RARCH_LOG("[Environ]: SET_HW_RENDER, context type: %s.\n", "vulkan");

    cb->get_current_framebuffer = video_driver_get_current_framebuffer;
    cb->get_proc_address = video_driver_get_proc_address;

    memcpy(hwr, cb, sizeof(*cb));
    RARCH_LOG("Reached end of SET_HW_RENDER.\n");

    return true;
}

bool retro_set_hw_render_context_negotiation_interface(void* data)
{
    video_driver_state_t* video_st =
        video_state_get_ptr();
    const struct retro_hw_render_context_negotiation_interface* iface =
        (const struct retro_hw_render_context_negotiation_interface*)data;
    RARCH_LOG("[Environ]: SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE.\n");
    video_st->hw_render_context_negotiation = iface;

    return true;
}
