/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *  Copyright (C) 2016-2019 - Brad Parker
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

#pragma once

#include <stdbool.h>
#include <Windows.h>

extern bool g_win32_inited;
extern unsigned g_win32_resize_width;
extern unsigned g_win32_resize_height;
extern bool g_win32_restore_desktop;

void win32_check_window(void* data,
    bool* quit, bool* resize,
    unsigned* width, unsigned* height);

HWND win32_get_window(void);

void win32_monitor_info(void* data, void* hm_data, unsigned* mon_id);

void win32_monitor_from_window(void);

void win32_destroy_window(void);

void win32_monitor_get_info(void);

void win32_window_reset(void);

void win32_monitor_init(void);

bool win32_set_video_mode(void* data,
    unsigned width, unsigned height,
    bool fullscreen);

void win32_get_video_output_size(unsigned* width, unsigned* height, char* desc, size_t desc_len);

float win32_get_refresh_rate(void* data);

bool win32_has_focus(void* data);