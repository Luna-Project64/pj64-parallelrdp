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

#include "video_driver.h"
#include "retroarch.h"
#include "video_display_server.h"
#include "compat_strl.h"

#include "driver.h"

#include <string.h>

static video_driver_state_t video_driver_st = { 0 };
static const video_display_server_t* current_display_server = &dispserv_win32;
static struct string_list* gpu_list = NULL;

video_driver_state_t* video_state_get_ptr(void)
{
	return &video_driver_st;
}

bool video_driver_find_driver(
    void* settings_data,
    const char* prefix, bool verbosity_enabled)
{
    video_driver_state_t* video_st = &video_driver_st;
    video_st->current_video = &video_vulkan;
    return true;
}

void video_driver_set_cached_frame_ptr(const void* data)
{
    video_driver_state_t* video_st = &video_driver_st;
    video_st->frame_cache_data = data;
}

void video_driver_free_hw_context(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    VIDEO_DRIVER_CONTEXT_LOCK(video_st);

    if (video_st->hw_render.context_destroy)
        video_st->hw_render.context_destroy();

    memset(&video_st->hw_render, 0, sizeof(video_st->hw_render));

    VIDEO_DRIVER_CONTEXT_UNLOCK(video_st);

    video_st->hw_render_context_negotiation = NULL;
}

void video_driver_lock_new(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    VIDEO_DRIVER_LOCK_FREE(video_st);
    if (!video_st->display_lock)
        video_st->display_lock = slock_new();
    retro_assert(video_st->display_lock);

    if (!video_st->context_lock)
        video_st->context_lock = slock_new();
    retro_assert(video_st->context_lock);
}

void video_driver_set_aspect_ratio_value(float value)
{
    video_driver_state_t* video_st = &video_driver_st;
    video_st->aspect_ratio = value;
}

bool video_driver_get_viewport_info(struct video_viewport* viewport)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (!video_st->current_video || !video_st->current_video->viewport_info)
        return false;
    video_st->current_video->viewport_info(video_st->data, viewport);
    return true;
}

static bool get_metrics_null(void* data, enum display_metric_types type,
    float* value) {
    return false;
}

void video_context_driver_reset(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (!video_st->current_video_context.get_metrics)
        video_st->current_video_context.get_metrics = get_metrics_null;
}

void* video_display_server_init()
{
    video_driver_state_t* video_st = &video_driver_st;
    video_display_server_destroy();

    current_display_server = &dispserv_win32;
    if (current_display_server)
    {
        if (current_display_server->init)
            video_st->current_display_server_data = current_display_server->init();
    }

    return video_st->current_display_server_data;
}

void video_display_server_destroy(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (current_display_server)
        if (video_st->current_display_server_data)
            current_display_server->destroy(video_st->current_display_server_data);
}

bool video_driver_init_internal(bool* video_is_threaded, bool verbosity_enabled)
{
    video_info_t video;
    unsigned max_dim, scale, width, height;
    video_viewport_t* custom_vp = NULL;
    static uint16_t dummy_pixels[32] = { 0 };
    video_driver_state_t* video_st = &video_driver_st;
    struct retro_game_geometry* geom = &video_st->av_info.geometry;
    int video_driver_pix_fmt = 0;
    settings_t* settings = config_get_ptr();

    video_viewport_t video_viewport_custom;
    custom_vp = &video_viewport_custom;

    max_dim = MAX(geom->max_width, geom->max_height);
    scale = next_pow2(max_dim) / RARCH_SCALE_BASE;
    scale = MAX(scale, 1);

    video_driver_set_aspect_ratio_value(1.f);

    if (settings->bools.video_fullscreen)
    {
        width = settings->uints.video_fullscreen_x;
        height = settings->uints.video_fullscreen_y;
    }
    else
    {
        width = settings->uints.window_position_width;
        height = settings->uints.window_position_height;
    }

    video.width = width;
    video.height = height;
    video.fullscreen = settings->bools.video_fullscreen;
    video.vsync = settings->bools.video_vsync;
    video.force_aspect = settings->bools.video_force_aspect;
    video.swap_interval = settings->uints.video_swap_interval;
    video.adaptive_vsync = settings->bools.video_adaptive_vsync;
    video.smooth = settings->bools.video_smooth;
    video.ctx_scaling = settings->bools.video_ctx_scaling;
    video.input_scale = scale;
    video.rgb32 = 1;
    video.parent = 0;

    video_st->started_fullscreen = video.fullscreen;
    /* Reset video frame count */
    video_st->frame_count = 0;

    video_driver_find_driver(settings,
        "video driver", verbosity_enabled);

    video_st->data = video_st->current_video->init(&video);

    if (!video_st->data)
    {
        RARCH_ERR("[Video]: Cannot open video driver ... Exiting ...\n");
        return false;
    }

    video_st->poke = NULL;
    if (video_st->current_video->poke_interface)
        video_st->current_video->poke_interface(
            video_st->data, &video_st->poke);

    if (video_st->current_video->viewport_info &&
        (!custom_vp->width ||
            !custom_vp->height))
    {
        /* Force custom viewport to have sane parameters. */
        custom_vp->width = width;
        custom_vp->height = height;

        video_driver_get_viewport_info(custom_vp);
    }

    video_context_driver_reset();

    video_display_server_init();

    return true;
}

const gfx_ctx_driver_t* video_context_driver_init(
    settings_t* settings,
    void* data,
    const gfx_ctx_driver_t* ctx,
    const char* ident,
    unsigned major,
    unsigned minor, bool hw_render_ctx,
    void** ctx_data)
{
    if (!ctx->bind_api(data, 0, major, minor))
    {
        RARCH_WARN("Failed to bind API (#%u, version %u.%u)"
            " on context driver \"%s\".\n",
            (unsigned)0, major, minor, ctx->ident);

        return NULL;
    }

    if (!(*ctx_data = ctx->init(data)))
        return NULL;

    if (ctx->bind_hw_render)
    {
        ctx->bind_hw_render(*ctx_data, 0);
    }

    return ctx;
}

static const gfx_ctx_driver_t* vk_context_driver_init_first(
    settings_t* settings,
    void* data,
    const char* ident, unsigned major,
    unsigned minor, bool hw_render_ctx, void** ctx_data)
{
    video_driver_state_t* video_st = &video_driver_st;

    const gfx_ctx_driver_t* ctx = video_context_driver_init(
        settings,
        data,
        &gfx_ctx_w_vk, ident,
        major, minor, hw_render_ctx, ctx_data);

    if (ctx)
        video_st->context_data = *ctx_data;

    return ctx;
}

const gfx_ctx_driver_t* video_context_driver_init_first(void* data,
    const char* ident, unsigned major,
    unsigned minor, bool hw_render_ctx, void** ctx_data)
{
    settings_t* settings = config_get_ptr();
    return vk_context_driver_init_first(
        settings,
        data, ident, major, minor, hw_render_ctx, ctx_data);
}

static void video_driver_reinit_context(settings_t* settings, int flags)
{
    /* RARCH_DRIVER_CTL_UNINIT clears the callback struct so we
     * need to make sure to keep a copy */
    struct retro_hw_render_callback hwr_copy;
    video_driver_state_t* video_st = &video_driver_st;
    struct retro_hw_render_callback* hwr =
        VIDEO_DRIVER_GET_HW_CONTEXT_INTERNAL(video_st);
    const struct retro_hw_render_context_negotiation_interface* iface =
        video_st->hw_render_context_negotiation;
    memcpy(&hwr_copy, hwr, sizeof(hwr_copy));

    driver_uninit(flags);

    memcpy(hwr, &hwr_copy, sizeof(*hwr));
    video_st->hw_render_context_negotiation = iface;

    drivers_init(settings, flags, false);
}

void video_driver_reinit()
{
    settings_t* settings = config_get_ptr();
    video_driver_state_t* video_st = &video_driver_st;
    struct retro_hw_render_callback* hwr =
        VIDEO_DRIVER_GET_HW_CONTEXT_INTERNAL(video_st);

    video_st->cache_context = (hwr->cache_context != false);
    video_st->cache_context_ack = false;
    video_driver_reinit_context(settings, 0);
    video_st->cache_context = false;
}

bool video_driver_is_video_cache_context(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    return video_st->cache_context;
}

void video_driver_free_internal(void)
{
    video_driver_state_t* video_st = &video_driver_st;

    if (!video_driver_is_video_cache_context())
        video_driver_free_hw_context();

    if (video_st->data
        && video_st->current_video
        && video_st->current_video->free)
        video_st->current_video->free(video_st->data);
}

static bool video_context_driver_get_metrics_null(
    void* data, enum display_metric_types type,
    float* value) {
    return false;
}

void video_context_driver_destroy(gfx_ctx_driver_t* ctx_driver)
{
    if (!ctx_driver)
        return;

    ctx_driver->init = NULL;
    ctx_driver->bind_api = NULL;
    ctx_driver->swap_interval = NULL;
    ctx_driver->set_video_mode = NULL;
    ctx_driver->get_video_size = NULL;
    ctx_driver->get_video_output_size = NULL;
    ctx_driver->get_video_output_prev = NULL;
    ctx_driver->get_video_output_next = NULL;
    ctx_driver->get_metrics =
        video_context_driver_get_metrics_null;
    ctx_driver->translate_aspect = NULL;
    ctx_driver->update_window_title = NULL;
    ctx_driver->check_window = NULL;
    ctx_driver->set_resize = NULL;
    ctx_driver->suppress_screensaver = NULL;
    ctx_driver->swap_buffers = NULL;
    ctx_driver->input_driver = NULL;
    ctx_driver->get_proc_address = NULL;
    ctx_driver->image_buffer_init = NULL;
    ctx_driver->image_buffer_write = NULL;
    ctx_driver->show_mouse = NULL;
    ctx_driver->ident = NULL;
    ctx_driver->get_flags = NULL;
    ctx_driver->set_flags = NULL;
    ctx_driver->bind_hw_render = NULL;
    ctx_driver->get_context_data = NULL;
    ctx_driver->make_current = NULL;
}

void video_context_driver_free(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    video_context_driver_destroy(&video_st->current_video_context);
    video_st->context_data = NULL;
}

void video_driver_cached_frame_get(const void** data, unsigned* width,
    unsigned* height, size_t* pitch)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (data)
        *data = video_st->frame_cache_data;
    if (width)
        *width = video_st->frame_cache_width;
    if (height)
        *height = video_st->frame_cache_height;
    if (pitch)
        *pitch = video_st->frame_cache_pitch;
}

struct retro_hw_render_callback* video_driver_get_hw_context(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    return VIDEO_DRIVER_GET_HW_CONTEXT_INTERNAL(video_st);
}

bool video_context_driver_set(const gfx_ctx_driver_t* data)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (!data)
        return false;
    video_st->current_video_context = *data;
    video_context_driver_reset();
    return true;
}

static bool video_driver_get_flags(gfx_ctx_flags_t* flags)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (!video_st->poke || !video_st->poke->get_flags)
        return false;
    flags->flags = video_st->poke->get_flags(video_st->data);
    return true;
}

bool video_context_driver_get_flags(gfx_ctx_flags_t* flags)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (!video_st->current_video_context.get_flags)
        return false;

    if (video_st->deferred_video_context_driver_set_flags)
    {
        flags->flags =
            video_st->deferred_flag_data.flags;
        video_st->deferred_video_context_driver_set_flags = false;
        return true;
    }

    flags->flags = video_st->current_video_context.get_flags(
        video_st->context_data);
    return true;
}

bool video_driver_test_all_flags(enum display_flags testflag)
{
    gfx_ctx_flags_t flags;

    if (video_driver_get_flags(&flags))
        if (BIT32_GET(flags.flags, testflag))
            return true;

    if (video_context_driver_get_flags(&flags))
        if (BIT32_GET(flags.flags, testflag))
            return true;

    return false;
}

void video_driver_get_size(unsigned* width, unsigned* height)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (width)
        *width = video_st->width;
    if (height)
        *height = video_st->height;
}

void video_driver_set_size(unsigned width, unsigned height)
{
    video_driver_state_t* video_st = &video_driver_st;
    video_st->width = width;
    video_st->height = height;
}

float video_driver_get_aspect_ratio(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    return video_st->aspect_ratio;
}

bool video_context_driver_get_refresh_rate(float* refresh_rate)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (!video_st->current_video_context.get_refresh_rate || !refresh_rate)
        return false;
    if (!video_st->context_data)
        return false;

    {
        float refresh_holder = 0;
        if (refresh_rate)
            refresh_holder =
            video_st->current_video_context.get_refresh_rate(
                video_st->context_data);

        *refresh_rate = refresh_holder;
    }

    return true;
}

const struct retro_hw_render_context_negotiation_interface
* video_driver_get_context_negotiation_interface(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    return video_st->hw_render_context_negotiation;
}

void video_driver_cached_frame(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    retro_video_refresh(
        (video_st->frame_cache_data != RETRO_HW_FRAME_BUFFER_VALID)
        ? video_st->frame_cache_data
        : NULL,
        video_st->frame_cache_width,
        video_st->frame_cache_height,
        video_st->frame_cache_pitch);
}

/* string list stays owned by the caller and must be available at
 * all times after the video driver is inited */
void video_driver_set_gpu_api_devices(int num, struct string_list* list)
{
    gpu_list = list;
}

void video_driver_set_gpu_device_string(const char* str)
{
    video_driver_state_t* video_st = &video_driver_st;
    strlcpy(video_st->gpu_device_string, str,
        sizeof(video_st->gpu_device_string));
}

const char* video_driver_get_gpu_device_string(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    return video_st->gpu_device_string;
}

void video_driver_set_video_cache_context_ack(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    video_st->cache_context_ack = true;
}

void video_driver_set_gpu_api_version_string(const char* str)
{
    video_driver_state_t* video_st = &video_driver_st;
    strlcpy(video_st->gpu_api_version_string, str,
        sizeof(video_st->gpu_api_version_string));
}

uintptr_t video_driver_get_current_framebuffer(void)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (video_st->poke
        && video_st->poke->get_current_framebuffer)
        return video_st->poke->get_current_framebuffer(video_st->data);
    return 0;
}

retro_proc_address_t video_driver_get_proc_address(const char* sym)
{
    video_driver_state_t* video_st = &video_driver_st;
    if (video_st->poke
        && video_st->poke->get_proc_address)
        return video_st->poke->get_proc_address(video_st->data, sym);
    return NULL;
}

void video_driver_frame(const void* data, unsigned width,
    unsigned height, size_t pitch)
{
    char status_text[128];
    static char video_driver_msg[256];
    static float last_fps, frame_time;
    static uint64_t last_used_memory, last_total_memory;
    /* Initialise 'last_frame_duped' to 'true'
     * to ensure that the first frame is rendered */
    static bool last_frame_duped = true;
    bool render_frame = true;
    video_driver_state_t* video_st = &video_driver_st;
    bool video_driver_active = video_st->active;

    status_text[0] = '\0';
    video_driver_msg[0] = '\0';

    if (!video_driver_active)
        return;

    if (data)
        video_st->frame_cache_data = data;
    video_st->frame_cache_width = width;
    video_st->frame_cache_height = height;
    video_st->frame_cache_pitch = pitch;

    last_frame_duped = !data;
    if (render_frame && video_st->current_video && video_st->current_video->frame)
        video_st->active = video_st->current_video->frame(
            video_st->data, data, width, height,
            video_st->frame_count, (unsigned)pitch,
            video_driver_msg);

    video_st->frame_count++;
}
