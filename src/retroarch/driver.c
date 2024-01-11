/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2021 - Daniel De Matteis
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

#include "driver.h"

#include "video_driver.h"
#include "gfx_display.h"

void drivers_init(
    settings_t* settings,
    int flags,
    bool verbosity_enabled)
{
    video_driver_state_t
        * video_st = video_state_get_ptr();
    bool video_is_threaded = false;
    gfx_display_t* p_disp = disp_get_ptr();

    /* Initialize video driver */
    {
        struct retro_hw_render_callback* hwr =
            VIDEO_DRIVER_GET_HW_CONTEXT_INTERNAL(video_st);

        video_driver_lock_new();
        video_driver_set_cached_frame_ptr(NULL);
        if (!video_driver_init_internal(&video_is_threaded,
            verbosity_enabled))
            retroarch_fail(1, "video_driver_init_internal()");

        if (!video_st->cache_context_ack
            && hwr->context_reset)
            hwr->context_reset();
    }

#if 0
    core_info_init_current_core();
#endif

    gfx_display_init_first_driver(p_disp, video_is_threaded);
}

void driver_uninit(int flags)
{
    video_driver_state_t
        * video_st = video_state_get_ptr();

    video_driver_free_internal();
    VIDEO_DRIVER_LOCK_FREE(video_st);
    video_st->data = NULL;
    video_driver_set_cached_frame_ptr(NULL);

    video_st->data = NULL;
}
