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

#pragma once

#include "volk.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

enum retro_log_level
{
	RETRO_LOG_DEBUG = 0,
	RETRO_LOG_INFO,
	RETRO_LOG_WARN,
	RETRO_LOG_ERROR,

	RETRO_LOG_DUMMY = INT_MAX
};

typedef void (*retro_log_printf_t)(enum retro_log_level level, const char* fmt, ...);
typedef bool (*retro_environment_t)(unsigned cmd, void* data);

struct retro_vulkan_image
{
	VkImageView image_view;
	VkImageLayout image_layout;
	VkImageViewCreateInfo create_info;
};

typedef void (*retro_vulkan_set_image_t)(void* handle,
    const struct retro_vulkan_image* image,
    uint32_t num_semaphores,
    const VkSemaphore* semaphores,
    uint32_t src_queue_family);

typedef uint32_t(*retro_vulkan_get_sync_index_t)(void* handle);
typedef uint32_t(*retro_vulkan_get_sync_index_mask_t)(void* handle);
typedef void (*retro_vulkan_set_command_buffers_t)(void* handle,
    uint32_t num_cmd,
    const VkCommandBuffer* cmd);
typedef void (*retro_vulkan_wait_sync_index_t)(void* handle);
typedef void (*retro_vulkan_lock_queue_t)(void* handle);
typedef void (*retro_vulkan_unlock_queue_t)(void* handle);
typedef void (*retro_vulkan_set_signal_semaphore_t)(void* handle, VkSemaphore semaphore);

typedef const VkApplicationInfo* (*retro_vulkan_get_application_info_t)(void);

struct retro_vulkan_context
{
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    uint32_t queue_family_index;
    VkQueue presentation_queue;
    uint32_t presentation_queue_family_index;
};

typedef bool (*retro_vulkan_create_device_t)(
    struct retro_vulkan_context* context,
    VkInstance instance,
    VkPhysicalDevice gpu,
    VkSurfaceKHR surface,
    PFN_vkGetInstanceProcAddr get_instance_proc_addr,
    const char** required_device_extensions,
    unsigned num_required_device_extensions,
    const char** required_device_layers,
    unsigned num_required_device_layers,
    const VkPhysicalDeviceFeatures* required_features);

typedef void (*retro_vulkan_destroy_device_t)(void);

enum retro_hw_render_context_negotiation_interface_type
{
    RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN = 0,
    RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_DUMMY = INT_MAX
};

struct retro_hw_render_context_negotiation_interface_vulkan
{
    enum retro_hw_render_context_negotiation_interface_type interface_type;
    unsigned interface_version;
    retro_vulkan_get_application_info_t get_application_info;
    retro_vulkan_create_device_t create_device;
    retro_vulkan_destroy_device_t destroy_device;
};

enum retro_hw_render_interface_type
{
    RETRO_HW_RENDER_INTERFACE_VULKAN = 0,
};

struct retro_hw_render_interface_vulkan
{
    enum retro_hw_render_interface_type interface_type;
    unsigned interface_version;

    void* handle;

    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;

    PFN_vkGetDeviceProcAddr get_device_proc_addr;
    PFN_vkGetInstanceProcAddr get_instance_proc_addr;

    VkQueue queue;
    unsigned queue_index;

    retro_vulkan_set_image_t set_image;
    retro_vulkan_get_sync_index_t get_sync_index;
    retro_vulkan_get_sync_index_mask_t get_sync_index_mask;
    retro_vulkan_set_command_buffers_t set_command_buffers;
    retro_vulkan_wait_sync_index_t wait_sync_index;
    retro_vulkan_lock_queue_t lock_queue;
    retro_vulkan_unlock_queue_t unlock_queue;
    retro_vulkan_set_signal_semaphore_t set_signal_semaphore;
};

struct texture_image
{
    uint32_t* pixels;
    unsigned width;
    unsigned height;
    bool supports_rgba;
};

#define RETRO_HW_FRAME_BUFFER_VALID ((void*)-1)

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define BIT32_SET(a, bit)    ((a) |=  (UINT32_C(1) << ((bit) & 31)))
#define BIT32_CLEAR(a, bit)  ((a) &= ~(UINT32_C(1) << ((bit) & 31)))
#define BIT32_GET(a, bit)    (((a) >> ((bit) & 31)) & 1)
#define BIT32_CLEAR_ALL(a)   ((a) = 0)

static inline uint32_t next_pow2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef struct 
{
    struct
    {
        bool video_fullscreen;
        bool video_vsync;
        bool video_force_aspect;
        bool video_adaptive_vsync;
        bool video_smooth;
        bool video_ctx_scaling;
        bool video_scale_integer;
    } bools;

    struct
    {
        unsigned video_fullscreen_x;
        unsigned video_fullscreen_y;
        unsigned window_position_width;
        unsigned window_position_height;
        unsigned window_position_x;
        unsigned window_position_y;
        unsigned video_swap_interval;
        unsigned video_max_swapchain_images;
    } uints;

    struct
    {
        int vulkan_gpu_index;
    } ints;
} settings_t;

#define RARCH_ERR(...) retroarch_fail(1, __VA_ARGS__)
#define RARCH_WARN(...) retroarch_fail(1, __VA_ARGS__)
#define retro_assert(...) assert(__VA_ARGS__)
#define RARCH_LOG(...) retro_log(RETRO_LOG_INFO, __VA_ARGS__)

inline bool string_is_equal(const char* a, const char* b)
{
    return (a && b) ? !strcmp(a, b) : false;
}

#define string_is_equal_fast(a, b, size)     (memcmp(a, b, size) == 0)

#ifdef __cplusplus
extern "C" {
#endif
    void retro_video_refresh(const void* data, unsigned width, unsigned height, size_t pitch);
    bool retro_init(bool fs, unsigned width, unsigned height);
    void retro_deinit(void);
    void retro_reinit(void);

    void retroarch_fail(int num, const char* err, ...);

    settings_t* config_get_ptr(void);
    
    void retro_sleep(int);

    void* retro_get_hw_render_interface(void);
    bool retro_set_hw_render(void*);
    bool retro_set_hw_render_context_negotiation_interface(void*);

    void retro_log(enum retro_log_level level, const char* fmt, ...);

#ifdef __cplusplus
}
#endif
