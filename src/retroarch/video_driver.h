#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "rthreads.h"
#include "retroarch.h"
#include "string_list.h"

typedef struct gfx_ctx_flags
{
    uint32_t flags;
} gfx_ctx_flags_t;

typedef void (* retro_proc_address_t)(void);
typedef void (* retro_hw_context_reset_t)(void);
typedef uintptr_t(* retro_hw_get_current_framebuffer_t)(void);
typedef retro_proc_address_t(* retro_hw_get_proc_address_t)(const char* sym);

struct retro_hw_render_callback
{
    retro_hw_context_reset_t context_reset;
    retro_hw_get_current_framebuffer_t get_current_framebuffer;
    retro_hw_get_proc_address_t get_proc_address;
    bool bottom_left_origin;
    bool cache_context;
    unsigned version_major;
    unsigned version_minor;
    retro_hw_context_reset_t context_destroy;
};

#define VIDEO_DRIVER_LOCK(video_st) \
   if (video_st->display_lock) \
      slock_lock(video_st->display_lock)

#define VIDEO_DRIVER_UNLOCK(video_st) \
   if (video_st->display_lock) \
      slock_unlock(video_st->display_lock)

#define VIDEO_DRIVER_CONTEXT_LOCK(video_st) \
   if (video_st->context_lock) \
      slock_lock(video_st->context_lock)

#define VIDEO_DRIVER_CONTEXT_UNLOCK(video_st) \
   if (video_st->context_lock) \
      slock_unlock(video_st->context_lock)

#define VIDEO_DRIVER_LOCK_FREE(video_st) \
   slock_free(video_st->display_lock); \
   slock_free(video_st->context_lock); \
   video_st->display_lock = NULL; \
   video_st->context_lock = NULL

typedef struct video_info
{
    uintptr_t parent;

    int swap_interval;


    /* Width of window.
     * If fullscreen mode is requested,
     * a width of 0 means the resolution of the
     * desktop should be used. */
    unsigned width;

    /* Height of window.
     * If fullscreen mode is requested,
     * a height of 0 means the resolutiof the desktop should be used.
     */
    unsigned height;

    /*
     * input_scale defines the maximum size of the picture that will
     * ever be used with the frame callback.
     *
     * The maximum resolution is a multiple of 256x256 size (RARCH_SCALE_BASE),
     * so an input scale of 2 means you should allocate a texture or of 512x512.
     *
     * Maximum input size: RARCH_SCALE_BASE * input_scale
     */
    unsigned input_scale;

    bool adaptive_vsync;

    /* If true, applies bilinear filtering to the image,
     * otherwise nearest filtering. */
    bool smooth;

    bool ctx_scaling;

    bool is_threaded;

    /* Use 32bit RGBA rather than native RGB565/XBGR1555.
     *
     * XRGB1555 format is 16-bit and has byte ordering: 0RRRRRGGGGGBBBBB,
     * in native endian.
     *
     * ARGB8888 is AAAAAAAARRRRRRRRGGGGGGGGBBBBBBBB, native endian.
     * Alpha channel should be disregarded.
     * */
    bool rgb32;

    /* Launch in fullscreen mode instead of windowed mode. */
    bool fullscreen;

    /* Start with V-Sync enabled. */
    bool vsync;

    /* If true, the output image should have the aspect ratio
     * as set in aspect_ratio. */
    bool force_aspect;
} video_info_t;

/* msg is for showing a message on the screen
 * along with the video frame. */
typedef bool (*video_driver_frame_t)(void* data,
    const void* frame, unsigned width,
    unsigned height, uint64_t frame_count,
    unsigned pitch, const char* msg);

enum texture_filter_type
{
    TEXTURE_FILTER_LINEAR = 0,
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_MIPMAP_LINEAR,
    TEXTURE_FILTER_MIPMAP_NEAREST
};

#define RETRO_MEMORY_ACCESS_WRITE (1 << 0)
/* The core will write to the buffer provided by retro_framebuffer::data. */
#define RETRO_MEMORY_ACCESS_READ (1 << 1)
   /* The core will read from retro_framebuffer::data. */
#define RETRO_MEMORY_TYPE_CACHED (1 << 0)

struct retro_framebuffer
{
    void* data;                      /* The framebuffer which the core can render into.
                                        Set by frontend in GET_CURRENT_SOFTWARE_FRAMEBUFFER.
                                        The initial contents of data are unspecified. */
    unsigned width;                  /* The framebuffer width used by the core. Set by core. */
    unsigned height;                 /* The framebuffer height used by the core. Set by core. */
    size_t pitch;                    /* The number of bytes between the beginning of a scanline,
                                        and beginning of the next scanline.
                                        Set by frontend in GET_CURRENT_SOFTWARE_FRAMEBUFFER. */
    unsigned access_flags;           /* How the core will access the memory in the framebuffer.
                                        RETRO_MEMORY_ACCESS_* flags.
                                        Set by core. */
    unsigned memory_flags;           /* Flags telling core how the memory has been mapped.
                                        RETRO_MEMORY_TYPE_* flags.
                                        Set by frontend in GET_CURRENT_SOFTWARE_FRAMEBUFFER. */
};

struct retro_hw_render_interface
{
    enum retro_hw_render_interface_type interface_type;
    unsigned interface_version;
};

typedef struct video_poke_interface
{
    uint32_t(*get_flags)(void* data);
    uintptr_t(*load_texture)(void* video_data, void* data,
        bool threaded, enum texture_filter_type filter_type);
    void (*unload_texture)(void* data, bool threaded, uintptr_t id);
    void (*set_video_mode)(void* data, unsigned width,
        unsigned height, bool fullscreen);
    float (*get_refresh_rate)(void* data);
    void (*set_filtering)(void* data, unsigned index, bool smooth, bool ctx_scaling);
    void (*get_video_output_size)(void* data,
        unsigned* width, unsigned* height, char* desc, size_t desc_len);

    /* Move index to previous resolution */
    void (*get_video_output_prev)(void* data);

    /* Move index to next resolution */
    void (*get_video_output_next)(void* data);

    uintptr_t(*get_current_framebuffer)(void* data);
    retro_proc_address_t(*get_proc_address)(void* data, const char* sym);
    void (*set_aspect_ratio)(void* data, unsigned aspectratio_index);
    void (*apply_state_changes)(void* data);

    /* Update texture. */
    void (*set_texture_frame)(void* data, const void* frame, bool rgb32,
        unsigned width, unsigned height, float alpha);
    /* Enable or disable rendering. */
    void (*set_texture_enable)(void* data, bool enable, bool full_screen);
    void (*set_osd_msg)(void* data,
        const char* msg,
        const void* params, void* font);

    void (*show_mouse)(void* data, bool state);
    void (*grab_mouse_toggle)(void* data);

    struct video_shader* (*get_current_shader)(void* data);
    bool (*get_current_software_framebuffer)(void* data,
        struct retro_framebuffer* framebuffer);
    bool (*get_hw_render_interface)(void* data,
        const struct retro_hw_render_interface** iface);

    /* hdr settings */
    void (*set_hdr_max_nits)(void* data, float max_nits);
    void (*set_hdr_paper_white_nits)(void* data, float paper_white_nits);
    void (*set_hdr_contrast)(void* data, float contrast);
    void (*set_hdr_expand_gamut)(void* data, bool expand_gamut);
} video_poke_interface_t;

typedef struct video_viewport
{
    int x;
    int y;
    unsigned width;
    unsigned height;
    unsigned full_width;
    unsigned full_height;
} video_viewport_t;

typedef struct video_driver
{
    /* Should the video driver act as an input driver as well?
     * The video initialization might preinitialize an input driver
     * to override the settings in case the video driver relies on
     * input driver for event handling. */
    void* (*init)(const video_info_t* video);

    /* Updates frame on the screen.
     * Frame can be either XRGB1555, RGB565 or ARGB32 format
     * depending on rgb32 setting in video_info_t.
     * Pitch is the distance in bytes between two scanlines in memory.
     *
     * When msg is non-NULL,
     * it's a message that should be displayed to the user. */
    video_driver_frame_t frame;

    /* Should we care about syncing to vblank? Fast forwarding.
     *
     * Requests nonblocking operation.
     *
     * True = VSync is turned off.
     * False = VSync is turned on.
     * */
    void (*set_nonblock_state)(void* data, bool toggle,
        bool adaptive_vsync_enabled,
        unsigned swap_interval);

    /* Is the window still active? */
    bool (*alive)(void* data);

    /* Does the window have focus? */
    bool (*focus)(void* data);

    /* Should the screensaver be suppressed? */
    bool (*suppress_screensaver)(void* data, bool enable);

    /* Does the graphics context support windowed mode? */
    bool (*has_windowed)(void* data);

    /* Sets shader. Might not be implemented. Will be moved to
     * poke_interface later. */
    bool (*set_shader)(void* data, int type,
        const char* path);

    /* Frees driver. */
    void (*free)(void* data);

    /* Human-readable identifier. */
    const char* ident;

    void (*set_viewport)(void* data, unsigned width, unsigned height,
        bool force_full, bool allow_rotate);

    void (*set_rotation)(void* data, unsigned rotation);
    void (*viewport_info)(void* data, struct video_viewport* vp);

    /* Reads out in BGR byte order (24bpp). */
    bool (*read_viewport)(void* data, uint8_t* buffer, bool is_idle);

    /* Returns a pointer to a newly allocated buffer that can
     * (and must) be passed to free() by the caller, containing a
     * copy of the current raw frame in the active pixel format
     * and sets width, height and pitch to the correct values. */
    void* (*read_frame_raw)(void* data, unsigned* width,
        unsigned* height, size_t* pitch);

    void (*poke_interface)(void* data, const video_poke_interface_t** iface);
    unsigned (*wrap_type_to_enum)(void);
} video_driver_t;

/* Base struct. All retro_hw_render_context_negotiation_interface_* types
 * contain at least these fields. */
struct retro_hw_render_context_negotiation_interface
{
    int interface_type;
    unsigned interface_version;
};

struct retro_game_geometry
{
    unsigned base_width;    /* Nominal video width of game. */
    unsigned base_height;   /* Nominal video height of game. */
    unsigned max_width;     /* Maximum possible width of game. */
    unsigned max_height;    /* Maximum possible height of game. */

    float    aspect_ratio;  /* Nominal aspect ratio of game. If
                             * aspect_ratio is <= 0.0, an aspect ratio
                             * of base_width / base_height is assumed.
                             * A frontend could override this setting,
                             * if desired. */
};

struct retro_system_av_info
{
    struct retro_game_geometry geometry;
};

enum display_metric_types
{
    DISPLAY_METRIC_NONE = 0,
    DISPLAY_METRIC_MM_WIDTH,
    DISPLAY_METRIC_MM_HEIGHT,
    DISPLAY_METRIC_DPI,
    DISPLAY_METRIC_PIXEL_WIDTH,
    DISPLAY_METRIC_PIXEL_HEIGHT
};

typedef void (*update_window_title_cb)(void*);
typedef bool (*get_metrics_cb)(void* data, enum display_metric_types type,
    float* value);
typedef bool (*set_resize_cb)(void*, unsigned, unsigned);

typedef void (*gfx_ctx_proc_t)(void);

typedef struct gfx_ctx_driver
{
    /* The opaque pointer is the underlying video driver data (e.g. gl_t for
     * OpenGL contexts). Although not advised, the context driver is allowed
     * to hold a pointer to it as the context never outlives the video driver.
     *
     * The context driver is responsible for it's own data.*/
    void* (*init)(void* video_driver);
    void (*destroy)(void* data);

    int (*get_api)(void* data);

    /* Which API to bind to. */
    bool (*bind_api)(void* video_driver, int,
        unsigned major, unsigned minor);

    /* Sets the swap interval. */
    void (*swap_interval)(void* data, int);

    /* Sets video mode. Creates a window, etc. */
    bool (*set_video_mode)(void*, unsigned, unsigned, bool);

    /* Gets current window size.
     * If not initialized yet, it returns current screen size. */
    void (*get_video_size)(void*, unsigned*, unsigned*);

    float (*get_refresh_rate)(void*);

    void (*get_video_output_size)(void*, unsigned*, unsigned*, char*, size_t);

    void (*get_video_output_prev)(void*);

    void (*get_video_output_next)(void*);

    get_metrics_cb get_metrics;

    /* Translates a window size to an aspect ratio.
     * In most cases this will be just width / height, but
     * some contexts will better know which actual aspect ratio is used.
     * This can be NULL to assume the default behavior.
     */
    float (*translate_aspect)(void*, unsigned, unsigned);

    /* Asks driver to update window title (FPS, etc). */
    update_window_title_cb update_window_title;

    /* Queries for resize and quit events.
     * Also processes events. */
    void (*check_window)(void*, bool*, bool*,
        unsigned*, unsigned*);

    /* Acknowledge a resize event. This is needed for some APIs.
     * Most backends will ignore this. */
    set_resize_cb set_resize;

    /* Checks if window has input focus. */
    bool (*has_focus)(void*);

    /* Should the screensaver be suppressed? */
    bool (*suppress_screensaver)(void* data, bool enable);

    /* Checks if context driver has windowed support. */
    bool has_windowed;

    /* Swaps buffers. VBlank sync depends on
     * earlier calls to swap_interval. */
    void (*swap_buffers)(void*);

    /* Most video backends will want to use a certain input driver.
     * Checks for it here. */
    void (*input_driver)(void*, const char*, void**, void**);

    /* Wraps whatever gl_proc_address() there is.
     * Does not take opaque, to avoid lots of ugly wrapper code. */
    gfx_ctx_proc_t(*get_proc_address)(const char*);

    /* Returns true if this context supports EGLImage buffers for
     * screen drawing and was initalized correctly. */
    bool (*image_buffer_init)(void*, const video_info_t*);

    /* Writes the frame to the EGLImage and sets image_handle to it.
     * Returns true if a new image handle is created.
     * Always returns true the first time it's called for a new index.
     * The graphics core must handle a change in the handle correctly. */
    bool (*image_buffer_write)(void*, const void* frame, unsigned width,
        unsigned height, unsigned pitch, bool rgb32,
        unsigned index, void** image_handle);

    /* Shows or hides mouse. Can be NULL if context doesn't
     * have a concept of mouse pointer. */
    void (*show_mouse)(void* data, bool state);

    /* Human readable string. */
    const char* ident;

    uint32_t(*get_flags)(void* data);

    void     (*set_flags)(void* data, uint32_t flags);

    /* Optional. Binds HW-render offscreen context. */
    void (*bind_hw_render)(void* data, bool enable);

    /* Optional. Gets base data for the context which is used by the driver.
     * This is mostly relevant for graphics APIs such as Vulkan
     * which do not have global context state. */
    void* (*get_context_data)(void* data);

    /* Optional. Makes driver context (only GL right now)
     * active for this thread. */
    void (*make_current)(bool release);
} gfx_ctx_driver_t;

typedef struct
{
    struct retro_system_av_info av_info; /* double alignment */
    // RETRO_PIXEL_FORMAT_XRGB8888
    bool active;
    struct retro_hw_render_callback hw_render;
    const void* frame_cache_data;
    unsigned frame_cache_width;
    unsigned frame_cache_height;
    size_t frame_cache_pitch;
    unsigned width;
    unsigned height;

    /* Set to true by driver if context caching succeeded. */
    bool cache_context_ack;
    bool deferred_video_context_driver_set_flags;
    video_driver_t* current_video;
    gfx_ctx_flags_t deferred_flag_data;          /* uint32_t alignment */

    const struct
        retro_hw_render_context_negotiation_interface*
        hw_render_context_negotiation;

    const video_poke_interface_t* poke;
    gfx_ctx_driver_t current_video_context;               /* ptr alignment */

    float aspect_ratio;
    void* data;
    void* context_data;
    void* current_display_server_data;

    bool started_fullscreen;
    bool cache_context;
    unsigned frame_count;

    char gpu_device_string[128];
    char gpu_api_version_string[128];

    void* display_lock;
    void* context_lock;
} video_driver_state_t;

enum display_flags
{
    GFX_CTX_FLAGS_NONE = 0,
    GFX_CTX_FLAGS_GL_CORE_CONTEXT,
    GFX_CTX_FLAGS_MULTISAMPLING,
    GFX_CTX_FLAGS_CUSTOMIZABLE_SWAPCHAIN_IMAGES,
    GFX_CTX_FLAGS_HARD_SYNC,
    GFX_CTX_FLAGS_BLACK_FRAME_INSERTION,
    GFX_CTX_FLAGS_MENU_FRAME_FILTERING,
    GFX_CTX_FLAGS_ADAPTIVE_VSYNC,
    GFX_CTX_FLAGS_SHADERS_GLSL,
    GFX_CTX_FLAGS_SHADERS_CG,
    GFX_CTX_FLAGS_SHADERS_HLSL,
    GFX_CTX_FLAGS_SHADERS_SLANG,
    GFX_CTX_FLAGS_SCREENSHOTS_SUPPORTED,
    GFX_CTX_FLAGS_OVERLAY_BEHIND_MENU_SUPPORTED
};

#ifndef GFX_MAX_SHADERS
#define GFX_MAX_SHADERS 64
#endif

#ifndef GFX_MAX_TEXTURES
#define GFX_MAX_TEXTURES 64
#endif

#ifndef GFX_MAX_PARAMETERS
#define GFX_MAX_PARAMETERS 1024
#endif

#ifndef GFX_MAX_FRAME_HISTORY
#define GFX_MAX_FRAME_HISTORY 128
#endif

/* This is pretty big, shouldn't be put on the stack.
 * Avoid lots of allocation for convenience. */
struct video_shader_parameter
{
    int pass;
    float current;
    float minimum;
    float initial;
    float maximum;
    float step;
    char id[64];
    char desc[64];
};

#define PATH_MAX_LENGTH 4096

enum gfx_scale_type
{
    RARCH_SCALE_INPUT = 0,
    RARCH_SCALE_ABSOLUTE,
    RARCH_SCALE_VIEWPORT
};

struct gfx_fbo_scale
{
    unsigned abs_x;
    unsigned abs_y;
    float scale_x;
    float scale_y;
    enum gfx_scale_type type_x;
    enum gfx_scale_type type_y;
    bool fp_fbo;
    bool srgb_fbo;
    bool valid;
};

enum gfx_wrap_type
{
    RARCH_WRAP_BORDER = 0, /* Kinda deprecated, but keep as default.
                              Will be translated to EDGE in GLES. */
    RARCH_WRAP_DEFAULT = RARCH_WRAP_BORDER,
    RARCH_WRAP_EDGE,
    RARCH_WRAP_REPEAT,
    RARCH_WRAP_MIRRORED_REPEAT,
    RARCH_WRAP_MAX
};

struct video_shader_pass
{
    struct gfx_fbo_scale fbo; /* unsigned alignment */
    unsigned filter;
    unsigned frame_count_mod;
    enum gfx_wrap_type wrap;
    struct
    {
        struct
        {
            char* vertex; /* Dynamically allocated. Must be free'd. */
            char* fragment; /* Dynamically allocated. Must be free'd. */
        } string;
        char path[PATH_MAX_LENGTH];
    } source;
    char alias[64];
    bool mipmap;
    bool feedback;
};

struct video_shader_lut
{
    unsigned filter;
    enum gfx_wrap_type wrap;
    char id[64];
    char path[PATH_MAX_LENGTH];
    bool mipmap;
};

struct video_shader
{
    struct video_shader_parameter parameters[GFX_MAX_PARAMETERS]; /* int alignment */
    /* If < 0, no feedback pass is used. Otherwise,
     * the FBO after pass #N is passed a texture to next frame. */
    int feedback_pass;
    int history_size;

    struct video_shader_pass pass[GFX_MAX_SHADERS]; /* unsigned alignment */
    struct video_shader_lut lut[GFX_MAX_TEXTURES];  /* unsigned alignment */
    unsigned passes;
    unsigned luts;
    unsigned num_parameters;
    unsigned variables;

    char prefix[64];

    /* Path to the root preset */
    char path[PATH_MAX_LENGTH];

    /* Path to the original preset loaded, if this is a preset with the #reference
     * directive then this will be different than the path*/
    char loaded_preset_path[PATH_MAX_LENGTH];

    bool modern; /* Only used for XML shaders. */
    /* indicative of whether shader was modified -
     * for instance from the menus */
    bool modified;
};

#define VIDEO_DRIVER_GET_HW_CONTEXT_INTERNAL(video_st) (&video_st->hw_render)

#define RARCH_SCALE_BASE 256

video_driver_state_t* video_state_get_ptr(void);

bool video_driver_find_driver(
    void* settings_data,
    const char* prefix, bool verbosity_enabled);

void video_driver_set_cached_frame_ptr(const void* data);

void video_driver_free_hw_context(void);

void video_driver_lock_new(void);

bool video_driver_init_internal(bool* video_is_threaded, bool verbosity_enabled);

void video_driver_reinit();

void video_driver_free_internal(void);

void video_context_driver_free(void);

void video_driver_cached_frame_get(const void** data, unsigned* width,
    unsigned* height, size_t* pitch);

struct retro_hw_render_callback* video_driver_get_hw_context(void);

const gfx_ctx_driver_t* video_context_driver_init_first(
    void* data, const char* ident,
    unsigned major, unsigned minor,
    bool hw_render_ctx, void** ctx_data);

bool video_context_driver_set(const gfx_ctx_driver_t* data);

void video_driver_get_size(unsigned* width, unsigned* height);

void video_driver_set_size(unsigned width, unsigned height);

float video_driver_get_aspect_ratio(void);

bool video_context_driver_get_refresh_rate(float* refresh_rate);

const struct retro_hw_render_context_negotiation_interface
* video_driver_get_context_negotiation_interface(void);

void video_driver_cached_frame(void);

bool video_driver_is_video_cache_context(void);

void video_driver_set_gpu_api_devices(
    int num, struct string_list* list);

void video_driver_set_gpu_device_string(const char* str);

const char* video_driver_get_gpu_device_string(void);

void video_driver_set_video_cache_context_ack(void);

bool video_driver_test_all_flags(enum display_flags testflag);

void video_driver_set_gpu_api_version_string(const char* str);

uintptr_t video_driver_get_current_framebuffer(void);

retro_proc_address_t video_driver_get_proc_address(const char* sym);

void video_driver_frame(const void* data, unsigned width,
    unsigned height, size_t pitch);

extern video_driver_t video_vulkan;
extern const gfx_ctx_driver_t gfx_ctx_w_vk;
